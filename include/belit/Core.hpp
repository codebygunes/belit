#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <string>

namespace belit {

// Core EVM Instruction Set Architecture (ISA) mapping for the lifter
enum class EvmOpcode : uint8_t {
    STOP = 0x00,
    ADD = 0x01,
    MUL = 0x02,
    SUB = 0x03,
    LT = 0x10,
    GT = 0x11,
    EQ = 0x14,
    POP = 0x50,
    JUMP = 0x56,
    JUMPI = 0x57,
    JUMPDEST = 0x5B,
    PUSH1 = 0x60,
    PUSH32 = 0x7F,
    DUP1 = 0x80,
    SWAP1 = 0x90,
    RETURN = 0xF3
};

// Standardized response structure for the bytecode lifting pipeline
struct LiftResult {
    bool success;
    std::string errorMessage;
    std::string llvmIR;
};

} // namespace belit