#include "belit/targets/WasmLifter.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <string>

namespace belit {

// --- LEB128 Decoding Utilities (Wasm's variable length integer format) ---
static uint64_t readULEB128(const std::vector<uint8_t>& data, size_t& offset) {
    uint64_t result = 0;
    unsigned shift = 0;
    while (offset < data.size()) {
        uint8_t byte = data[offset++];
        result |= (static_cast<uint64_t>(byte & 0x7F) << shift);
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    return result;
}

static int64_t readSLEB128(const std::vector<uint8_t>& data, size_t& offset) {
    int64_t result = 0;
    unsigned shift = 0;
    uint8_t byte;
    do {
        if (offset >= data.size()) break;
        byte = data[offset++];
        result |= (static_cast<int64_t>(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);

    if (shift < 64 && (byte & 0x40)) {
        result |= -(1ULL << shift);
    }
    return result;
}

WasmLifter::WasmLifter() {
    reset();
}

void WasmLifter::reset() {
    cfg_.clear();
}

const std::vector<BasicBlock>& WasmLifter::getCFG() const {
    return cfg_;
}

bool WasmLifter::parse(const std::vector<uint8_t>& rawBytecode) {
    reset();
    if (rawBytecode.size() < 8) {
        return false;
    }

    // 1. WASM Magic Number & Version Check
    if (rawBytecode[0] != 0x00 || rawBytecode[1] != 0x61 || 
        rawBytecode[2] != 0x73 || rawBytecode[3] != 0x6D) {
        return false; // Invalid Magic
    }
    
    uint32_t version = rawBytecode[4] | (rawBytecode[5] << 8) | 
                       (rawBytecode[6] << 16) | (rawBytecode[7] << 24);
    if (version != 1) {
        return false; // Unsupported Version
    }

    size_t offset = 8;
    
    // 2. Section Parsing Loop
    while (offset < rawBytecode.size()) {
        uint8_t sectionId = rawBytecode[offset++];
        uint32_t sectionSize = static_cast<uint32_t>(readULEB128(rawBytecode, offset));
        size_t nextSectionOffset = offset + sectionSize;

        // Section 10 is the Code section containing function bodies
        if (sectionId == 10) { 
            uint32_t numFunctions = static_cast<uint32_t>(readULEB128(rawBytecode, offset));
            
            for (uint32_t i = 0; i < numFunctions; ++i) {
                uint32_t bodySize = static_cast<uint32_t>(readULEB128(rawBytecode, offset));
                size_t functionEnd = offset + bodySize;
                
                // Parse local variables (skip for CFG lifting, but must be consumed)
                uint32_t localDeclCount = static_cast<uint32_t>(readULEB128(rawBytecode, offset));
                for (uint32_t j = 0; j < localDeclCount; ++j) {
                    readULEB128(rawBytecode, offset); // count
                    offset++; // value type
                }

                // 3. Instruction Parsing & CFG Construction
                BasicBlock currentBlock;
                currentBlock.startPc = offset;

                while (offset < functionEnd) {
                    size_t instPc = offset;
                    uint8_t opcode = rawBytecode[offset++];
                    Instruction inst;
                    inst.pc = instPc;
                    inst.type = OpCodeType::Unknown;

                    switch (opcode) {
                        case 0x00: inst.mnemonic = "UNREACHABLE"; break;
                        case 0x01: inst.mnemonic = "NOP"; break;
                        
                        // Structured Control Flow
                        case 0x02: inst.mnemonic = "BLOCK"; offset++; /* blocktype */ break;
                        case 0x03: inst.mnemonic = "LOOP"; offset++; /* blocktype */ break;
                        case 0x04: inst.mnemonic = "IF"; offset++; /* blocktype */ break;
                        case 0x05: inst.mnemonic = "ELSE"; break;
                        case 0x0B: inst.mnemonic = "END"; break;
                        case 0x0C: inst.mnemonic = "BR"; inst.type = OpCodeType::Jump; readULEB128(rawBytecode, offset); break;
                        case 0x0D: inst.mnemonic = "BR_IF"; inst.type = OpCodeType::Jump; readULEB128(rawBytecode, offset); break;
                        case 0x0F: inst.mnemonic = "RETURN"; inst.type = OpCodeType::Return; break;
                        
                        // --- FVM HOST CALL DESTEĞİ (EKLENDİ) ---
                        case 0x10: { 
                            inst.mnemonic = "CALL"; 
                            inst.type = OpCodeType::Unknown; 
                            uint32_t funcIdx = static_cast<uint32_t>(readULEB128(rawBytecode, offset));
                            // Fonksiyon indeksini LLVMTranslator'ın okuyabilmesi için operandlara yazıyoruz
                            inst.operands = {
                                static_cast<uint8_t>(funcIdx & 0xFF),
                                static_cast<uint8_t>((funcIdx >> 8) & 0xFF),
                                static_cast<uint8_t>((funcIdx >> 16) & 0xFF),
                                static_cast<uint8_t>((funcIdx >> 24) & 0xFF)
                            };
                            break;
                        }
                        
                        // Variables
                        case 0x20: inst.mnemonic = "LOCAL_GET"; readULEB128(rawBytecode, offset); break;
                        case 0x21: inst.mnemonic = "LOCAL_SET"; readULEB128(rawBytecode, offset); break;
                        case 0x22: inst.mnemonic = "LOCAL_TEE"; readULEB128(rawBytecode, offset); break;

                        // Memory Operations
                        case 0x28: inst.mnemonic = "I32_LOAD"; readULEB128(rawBytecode, offset); readULEB128(rawBytecode, offset); break;
                        case 0x36: inst.mnemonic = "I32_STORE"; readULEB128(rawBytecode, offset); readULEB128(rawBytecode, offset); break;
                        case 0x3F: inst.mnemonic = "MEMORY_SIZE"; offset++; break; // Reserved byte
                        case 0x40: inst.mnemonic = "MEMORY_GROW"; offset++; break; // Reserved byte
                        
                        // Constants (Requires decoding LEB128 and passing to LLVM IR)
                        case 0x41: { 
                            inst.type = OpCodeType::Push;
                            inst.mnemonic = "I32_CONST";
                            int32_t val = static_cast<int32_t>(readSLEB128(rawBytecode, offset));
                            // Serialize value into operands for LLVMTranslator
                            inst.operands = {
                                static_cast<uint8_t>(val & 0xFF),
                                static_cast<uint8_t>((val >> 8) & 0xFF),
                                static_cast<uint8_t>((val >> 16) & 0xFF),
                                static_cast<uint8_t>((val >> 24) & 0xFF)
                            };
                            break;
                        }
                        
                        // Numeric Instructions
                        case 0x45: inst.mnemonic = "I32_EQZ"; break;
                        case 0x46: inst.mnemonic = "I32_EQ"; break;
                        case 0x47: inst.mnemonic = "I32_NE"; break;
                        case 0x6A: inst.type = OpCodeType::Add; inst.mnemonic = "I32_ADD"; break;
                        case 0x6B: inst.type = OpCodeType::Sub; inst.mnemonic = "I32_SUB"; break;
                        case 0x6C: inst.mnemonic = "I32_MUL"; break;
                        case 0x6D: inst.mnemonic = "I32_DIV_S"; break;
                        case 0x6E: inst.mnemonic = "I32_DIV_U"; break;
                        case 0x71: inst.mnemonic = "I32_AND"; break;
                        case 0x72: inst.mnemonic = "I32_OR"; break;
                        case 0x73: inst.mnemonic = "I32_XOR"; break;

                        default:
                            inst.mnemonic = "UNKNOWN_WASM_" + std::to_string(opcode);
                            break;
                    }

                    currentBlock.instructions.push_back(inst);

                    // Block boundaries for CFG (Wasm structured flow ends block on branches/ends)
                    if (inst.mnemonic == "BR" || inst.mnemonic == "BR_IF" || 
                        inst.mnemonic == "RETURN" || inst.mnemonic == "END" ||
                        inst.mnemonic == "UNREACHABLE") {
                        
                        currentBlock.endPc = offset;
                        cfg_.push_back(currentBlock);
                        currentBlock = BasicBlock();
                        currentBlock.startPc = offset;
                    }
                }
            }
        }
        offset = nextSectionOffset; // Skip unknown sections quickly
    }

    return !cfg_.empty();
}

} // namespace belit