#include <gtest/gtest.h>
#include "belit/targets/EVMLifter.hpp"
#include "belit/ir/LLVMTranslator.hpp"
#include "belit/verifier/Z3SymbolicEngine.hpp"

TEST(Z3SymbolicEngineTest, RigorousSymbolicModelExtraction) {
    belit::EVMLifter lifter;
    
    // Malicious bytecode sequence designed to trigger an arithmetic overflow
    std::vector<uint8_t> maliciousBytecode = {
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // PUSH32 max_uint256
        0x60, 0x01,                                           // PUSH1 0x01
        0x01                                                  // ADD (Overflow)
    };
    ASSERT_TRUE(lifter.parse(maliciousBytecode));

    // YENİ: Z3 motoruna göndermeden önce kesinlikle LLVM IR'a çevirmeliyiz
    belit::LLVMTranslator translator;
    auto module = translator.translateToLLVM(lifter.getCFG(), "z3_test_module");
    ASSERT_NE(module, nullptr);

    belit::Z3SymbolicEngine engine;
    
    // YENİ: Z3 motoru artık lifter.getCFG() değil, doğrudan *module okuyor!
    auto verificationResult = engine.verify(*module);
    
    // Strict validation assertions matching engine outputs
    EXPECT_FALSE(verificationResult.isSafe);
    ASSERT_FALSE(verificationResult.discoveredViolations.empty());
    
    // Check for violation message substring compatibility
    bool foundOverflowMsg = false;
    for (const auto& violation : verificationResult.discoveredViolations) {
        if (violation.find("Overflow") != std::string::npos || violation.find("constraint") != std::string::npos) {
            foundOverflowMsg = true;
            break;
        }
    }
    EXPECT_TRUE(foundOverflowMsg);
}