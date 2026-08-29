#include <gtest/gtest.h>
#include "belit/targets/EVMLifter.hpp"
#include "belit/targets/WasmLifter.hpp"
#include <vector>

TEST(LifterTest, AdvancedRealWorldBytecodeDisassemblyAndCFG) {
    belit::EVMLifter lifter;
    // Real multi-byte instruction sequence: PUSH2 0x1234, PUSH1 0x0A, ADD, JUMP, JUMPDEST, RETURN
    std::vector<uint8_t> complexBytecode = {
        0x61, 0x12, 0x34, // PUSH2 0x1234
        0x60, 0x0A,       // PUSH1 10
        0x01,             // ADD
        0x56,             // JUMP
        0x5B,             // JUMPDEST
        0xF3              // RETURN
    };
    
    bool success = lifter.parse(complexBytecode);
    ASSERT_TRUE(success);
    
    const auto& cfg = lifter.getCFG();
    ASSERT_FALSE(cfg.empty());
    
    bool structuralIntegrityPassed = false;
    for (const auto& block : cfg) {
        if (!block.instructions.empty()) {
            structuralIntegrityPassed = true;
        }
    }
    EXPECT_TRUE(structuralIntegrityPassed);
}

TEST(LifterTest, HandlesEmptyBytecodeRobustly) {
    belit::EVMLifter lifter;
    std::vector<uint8_t> emptyBytecode = {};
    EXPECT_FALSE(lifter.parse(emptyBytecode));
    EXPECT_TRUE(lifter.getCFG().empty());
}

TEST(LifterTest, WasmRealBytecodeLEB128AndSections) {
    belit::WasmLifter lifter;
    
    // Minimal valid Wasm module:
    // 1. Magic "\0asm" + Version 1
    // 2. Section 10 (Code): 1 function, i32.const 42, i32.const 10, i32.add, end
    std::vector<uint8_t> realWasm = {
        0x00, 0x61, 0x73, 0x6D, // Magic
        0x01, 0x00, 0x00, 0x00, // Version 1
        0x0A,                   // Section 10 (Code) ID
        0x09,                   // Section payload size (9 bytes)
        0x01,                   // Number of functions: 1
        0x07,                   // Function body size: 7 bytes
        0x00,                   // Local declarations count: 0
        0x41, 0x2A,             // I32_CONST 42 (0x2A in LEB128)
        0x41, 0x0A,             // I32_CONST 10 (0x0A in LEB128)
        0x6A,                   // I32_ADD
        0x0B                    // END
    };
    
    bool success = lifter.parse(realWasm);
    ASSERT_TRUE(success);
    const auto& cfg = lifter.getCFG();
    ASSERT_FALSE(cfg.empty());
    
    // Confirm that the CFG is correctly parsed and blocked in preparation for SSA
    const auto& block = cfg[0];
    ASSERT_EQ(block.instructions.size(), 4);
    
    // Verify that the value 42 is decoded from SLEB128 and correctly transferred to the 4-byte operand
    EXPECT_EQ(block.instructions[0].mnemonic, "I32_CONST");
    EXPECT_EQ(block.instructions[0].operands.size(), 4);
    EXPECT_EQ(block.instructions[0].operands[0], 42);
    
    EXPECT_EQ(block.instructions[1].mnemonic, "I32_CONST");
    EXPECT_EQ(block.instructions[1].operands[0], 10);
    EXPECT_EQ(block.instructions[2].mnemonic, "I32_ADD");
    
    // Verify that the block boundary is correctly closed with the END instruction
    EXPECT_EQ(block.instructions[3].mnemonic, "END");
}

TEST(LifterTest, EVMStrictHaltOnUnknownOpcode) {
    belit::EVMLifter lifter;
    
    // 0x60 0x01 (PUSH1 1), followed by 0xFC (UNMAPPED/UNKNOWN), then 0x00 (STOP)
    std::vector<uint8_t> corruptedBytecode = {
        0x60, 0x01, 
        0xFC,       // STRICT HALT TRIGGER: 0xFC is an invalid EVM opcode
        0x00
    };
    
    // The lifter MUST reject the entire payload immediately, leaving the CFG empty.
    bool success = lifter.parse(corruptedBytecode);
    EXPECT_FALSE(success);
    EXPECT_TRUE(lifter.getCFG().empty());
}

TEST(LifterTest, WasmStrictHaltOnUnknownOpcode) {
    belit::WasmLifter lifter;
    
    // Valid Magic & Version, but Section 10 contains an unmapped opcode (0xFE)
    std::vector<uint8_t> corruptedWasm = {
        0x00, 0x61, 0x73, 0x6D, // Magic
        0x01, 0x00, 0x00, 0x00, // Version 1
        0x0A,                   // Section 10 (Code) ID
        0x06,                   // Section payload size
        0x01,                   // Number of functions: 1
        0x04,                   // Function body size
        0x00,                   // Local declarations count: 0
        0xFE,                   // STRICT HALT TRIGGER: Unmapped opcode
        0x0B                    // END
    };
    
    // The lifter MUST reject the entire payload immediately, leaving the CFG empty.
    bool success = lifter.parse(corruptedWasm);
    EXPECT_FALSE(success);
    EXPECT_TRUE(lifter.getCFG().empty());
}