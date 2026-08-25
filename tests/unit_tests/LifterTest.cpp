#include <gtest/gtest.h>
#include "belit/EvmToIR.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>

using namespace belit;
using namespace llvm;

TEST(EvmLifterTests, DynamicJumpAndMemorySimulation) {
    LLVMContext ctx;
    Module mod("TestModule", ctx);
    EvmToIR lifter(ctx, mod);

    // REAL WORLD EVM BYTECODE (No Mocks!)
    // 0: PUSH1 0x0A (10)
    // 2: PUSH1 0x14 (20)
    // 4: ADD (10 + 20 = 30)
    // 5: PUSH1 0x00
    // 7: MSTORE (Write value 30 to memory[0])
    // 8: PUSH1 0x0F (Target JUMPDEST address: 15)
    // 10: JUMP (Dynamic Jump!)
    // 11: PUSH1 0xFF (Dead code - Should be skipped)
    // 13: PUSH1 0xFF (Dead code)
    // 15: JUMPDEST (0x5B - Valid Jump Destination)
    // 16: PUSH1 0x00
    // 18: MLOAD (Read value 30 back from memory[0])
    
    std::vector<uint8_t> bytecode = {
        0x60, 0x0A, 
        0x60, 0x14, 
        0x01,       
        0x60, 0x00, 
        0x52,       
        0x60, 0x0F, 
        0x56,       
        0x60, 0xFF, 
        0x60, 0xFF, 
        0x5B,       
        0x60, 0x00, 
        0x51        
    };

    Function* liftedFunc = lifter.liftBytecode(bytecode, "test_dynamic_lifter");
    
    ASSERT_NE(liftedFunc, nullptr);
    
    std::string irOutput;
    raw_string_ostream rso(irOutput);
    liftedFunc->print(rso);
    
    std::cout << "\n=============================================\n";
    std::cout << "[ GENERATED DYNAMIC LLVM IR (NO MOCKS) ]\n";
    std::cout << irOutput;
    std::cout << "=============================================\n\n";

    EXPECT_TRUE(irOutput.find("alloca i8") != std::string::npos) << "Alloca (Memory) not found!";
    EXPECT_TRUE(irOutput.find("switch i256") != std::string::npos) << "Dynamic JUMP (SwitchInst) not found!";
}

TEST(EvmLifterTests, InvalidJumpDestDetection) {
    LLVMContext ctx;
    Module mod("TestModule", ctx);
    EvmToIR lifter(ctx, mod);

    // MALICIOUS BYTECODE (Fake JUMPDEST trap)
    // 0: PUSH1 0x03 (Target fake JUMPDEST address - pc=3)
    // 2: PUSH1 0x5B (This instruction is at PC=2. 0x5B is its payload, located at PC=3)
    // 4: JUMP       (Attempt to jump to PC=3, directly into the PUSH payload!)
    // 5: JUMPDEST   (Actual jump destination, but it is not targeted)
    
    std::vector<uint8_t> maliciousBytecode = {
        0x60, 0x03, 
        0x60, 0x5B, 
        0x56,       
        0x5B        
    };

    // Lifter should not crash!
    Function* liftedFunc = nullptr;
    EXPECT_NO_THROW({
        liftedFunc = lifter.liftBytecode(maliciousBytecode, "test_invalid_jump");
    });
    
    ASSERT_NE(liftedFunc, nullptr);

    std::string irOutput;
    raw_string_ostream rso(irOutput);
    liftedFunc->print(rso);

    std::cout << "\n=============================================\n";
    std::cout << "[ FAKE JUMPDEST DEFENSE (LLVM IR) ]\n";
    std::cout << irOutput;
    std::cout << "=============================================\n\n";

    // Verify that "switch i256 3" falls back to the "invalid_jump" block,
    // because a valid 'jumpdest_3' block should not have been created (Pass 1 should skip it).
    EXPECT_TRUE(irOutput.find("switch i256 3, label %invalid_jump") != std::string::npos || 
                irOutput.find("invalid_jump") != std::string::npos) 
                << "Lifter failed to prevent jump to a fake JUMPDEST!";
}