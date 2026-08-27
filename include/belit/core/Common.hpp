#ifndef BELIT_COMMON_HPP
#define BELIT_COMMON_HPP

#include <vector>
#include <string>
#include <cstdint>

namespace belit {

// Extended OpCode Types
enum class OpCodeType {
    Push,
    Add,
    Sub,
    Jump,
    JumpDest,
    Return,
    Revert,  // Added Revert as required by LLVMTranslator
    Unknown
};

// Universal Virtual Machine (VM) Instruction Structure
struct Instruction {
    size_t pc;
    OpCodeType type;
    std::string mnemonic;
    std::vector<uint8_t> operands;
};

// Control Flow Graph (CFG) Basic Block Structure
struct BasicBlock {
    size_t startPc = 0;
    size_t endPc = 0;
    std::vector<Instruction> instructions;
};

// Output Structure of the Z3 Engine
struct VerificationResult {
    bool isSafe;
    std::vector<std::string> discoveredViolations;
    std::string report;
};

} // namespace belit

#endif // BELIT_COMMON_HPP