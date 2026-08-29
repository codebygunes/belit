#include "belit/targets/EVMLifter.hpp"
#include <string>

namespace belit {

EVMLifter::EVMLifter() {
    reset();
}

void EVMLifter::reset() {
    cfg_.clear();
}

const std::vector<BasicBlock>& EVMLifter::getCFG() const {
    return cfg_;
}

bool EVMLifter::parse(const std::vector<uint8_t>& rawBytecode) {
    reset();
    if (rawBytecode.empty()) {
        return false;
    }

    BasicBlock currentBlock;
    currentBlock.startPc = 0;

    for (size_t i = 0; i < rawBytecode.size(); ++i) {
        uint8_t op = rawBytecode[i];
        Instruction inst;
        inst.pc = i;
        inst.type = OpCodeType::Unknown; 

        // 1. PUSH, DUP, SWAP, LOG Operations (Dynamic size parsing)
        if (op >= 0x60 && op <= 0x7F) { // PUSH1 - PUSH32
            inst.type = OpCodeType::Push;
            inst.mnemonic = "PUSH" + std::to_string(op - 0x5F);
            size_t pushSize = op - 0x5F;
            for (size_t j = 0; j < pushSize && (i + 1) < rawBytecode.size(); ++j) {
                inst.operands.push_back(rawBytecode[++i]);
            }
        } else if (op >= 0x80 && op <= 0x8F) { // DUP1 - DUP16
            inst.mnemonic = "DUP" + std::to_string(op - 0x7F);
        } else if (op >= 0x90 && op <= 0x9F) { // SWAP1 - SWAP16
            inst.mnemonic = "SWAP" + std::to_string(op - 0x8F);
        } else if (op >= 0xA0 && op <= 0xA4) { // LOG0 - LOG4
            inst.mnemonic = "LOG" + std::to_string(op - 0xA0);
        } else {
            // 2. Comprehensive EVM Opcode Mapping
            switch (op) {
                case 0x00: inst.mnemonic = "STOP"; break;
                case 0x01: inst.type = OpCodeType::Add; inst.mnemonic = "ADD"; break;
                case 0x02: inst.mnemonic = "MUL"; break;
                case 0x03: inst.type = OpCodeType::Sub; inst.mnemonic = "SUB"; break;
                case 0x04: inst.mnemonic = "DIV"; break;
                case 0x05: inst.mnemonic = "SDIV"; break;
                case 0x06: inst.mnemonic = "MOD"; break;
                case 0x07: inst.mnemonic = "SMOD"; break;
                case 0x08: inst.mnemonic = "ADDMOD"; break;
                case 0x09: inst.mnemonic = "MULMOD"; break;
                case 0x0A: inst.mnemonic = "EXP"; break;
                case 0x0B: inst.mnemonic = "SIGNEXTEND"; break;
                
                case 0x10: inst.mnemonic = "LT"; break;
                case 0x11: inst.mnemonic = "GT"; break;
                case 0x12: inst.mnemonic = "SLT"; break;
                case 0x13: inst.mnemonic = "SGT"; break;
                case 0x14: inst.mnemonic = "EQ"; break;
                case 0x15: inst.mnemonic = "ISZERO"; break;
                case 0x16: inst.mnemonic = "AND"; break;
                case 0x17: inst.mnemonic = "OR"; break;
                case 0x18: inst.mnemonic = "XOR"; break;
                case 0x19: inst.mnemonic = "NOT"; break;
                case 0x1A: inst.mnemonic = "BYTE"; break;
                case 0x1B: inst.mnemonic = "SHL"; break;
                case 0x1C: inst.mnemonic = "SHR"; break;
                case 0x1D: inst.mnemonic = "SAR"; break;
                
                case 0x20: inst.mnemonic = "KECCAK256"; break;
                
                case 0x30: inst.mnemonic = "ADDRESS"; break;
                case 0x31: inst.mnemonic = "BALANCE"; break;
                case 0x32: inst.mnemonic = "ORIGIN"; break;
                case 0x33: inst.mnemonic = "CALLER"; break;
                case 0x34: inst.mnemonic = "CALLVALUE"; break;
                case 0x35: inst.mnemonic = "CALLDATALOAD"; break;
                case 0x36: inst.mnemonic = "CALLDATASIZE"; break;
                case 0x37: inst.mnemonic = "CALLDATACOPY"; break;
                case 0x38: inst.mnemonic = "CODESIZE"; break;
                case 0x39: inst.mnemonic = "CODECOPY"; break;
                case 0x3A: inst.mnemonic = "GASPRICE"; break;
                case 0x3B: inst.mnemonic = "EXTCODESIZE"; break;
                case 0x3C: inst.mnemonic = "EXTCODECOPY"; break;
                case 0x3D: inst.mnemonic = "RETURNDATASIZE"; break;
                case 0x3E: inst.mnemonic = "RETURNDATACOPY"; break;
                case 0x3F: inst.mnemonic = "EXTCODEHASH"; break;
                
                case 0x40: inst.mnemonic = "BLOCKHASH"; break;
                case 0x41: inst.mnemonic = "COINBASE"; break;
                case 0x42: inst.mnemonic = "TIMESTAMP"; break;
                case 0x43: inst.mnemonic = "NUMBER"; break;
                case 0x44: inst.mnemonic = "DIFFICULTY"; break;
                case 0x45: inst.mnemonic = "GASLIMIT"; break;
                case 0x46: inst.mnemonic = "CHAINID"; break;
                case 0x47: inst.mnemonic = "SELFBALANCE"; break;
                case 0x48: inst.mnemonic = "BASEFEE"; break;
                
                case 0x50: inst.mnemonic = "POP"; break;
                case 0x51: inst.mnemonic = "MLOAD"; break;
                case 0x52: inst.mnemonic = "MSTORE"; break;
                case 0x53: inst.mnemonic = "MSTORE8"; break;
                case 0x54: inst.mnemonic = "SLOAD"; break;
                case 0x55: inst.mnemonic = "SSTORE"; break;
                case 0x56: inst.type = OpCodeType::Jump; inst.mnemonic = "JUMP"; break;
                case 0x57: inst.type = OpCodeType::Jump; inst.mnemonic = "JUMPI"; break;
                case 0x58: inst.mnemonic = "PC"; break;
                case 0x59: inst.mnemonic = "MSIZE"; break;
                case 0x5A: inst.mnemonic = "GAS"; break;
                case 0x5B: inst.type = OpCodeType::JumpDest; inst.mnemonic = "JUMPDEST"; break;
                
                case 0xF0: inst.mnemonic = "CREATE"; break;
                case 0xF1: inst.mnemonic = "CALL"; break;
                case 0xF2: inst.mnemonic = "CALLCODE"; break;
                case 0xF3: inst.type = OpCodeType::Return; inst.mnemonic = "RETURN"; break;
                case 0xF4: inst.mnemonic = "DELEGATECALL"; break;
                case 0xF5: inst.mnemonic = "CREATE2"; break;
                case 0xFA: inst.mnemonic = "STATICCALL"; break;
                case 0xFD: inst.type = OpCodeType::Revert; inst.mnemonic = "REVERT"; break;
                case 0xFE: inst.mnemonic = "INVALID"; break;
                case 0xFF: inst.mnemonic = "SELFDESTRUCT"; break;
                
                default: 
                    // STRICT HALT & PROVE: No graceful degradation. 
                    // Unmapped opcode implies obfuscation attempt or corrupted payload.
                    // Reject parsing immediately.
                    reset();
                    return false;
            }
        }
        
        currentBlock.instructions.push_back(inst);

        // 3. Precise Control Flow Graph (CFG) Block Boundaries
        if (inst.mnemonic == "JUMP" || inst.mnemonic == "JUMPI" || 
            inst.mnemonic == "RETURN" || inst.mnemonic == "REVERT" || 
            inst.mnemonic == "STOP" || inst.mnemonic == "INVALID" || 
            inst.mnemonic == "SELFDESTRUCT" || (i + 1) == rawBytecode.size()) {
            
            currentBlock.endPc = i;
            cfg_.push_back(currentBlock);
            currentBlock = BasicBlock();
            currentBlock.startPc = i + 1;
        }
    }

    return !cfg_.empty();
}

} // namespace belit