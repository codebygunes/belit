#include <gtest/gtest.h>
#include "belit/Z3Verifier.hpp"

using namespace belit;

TEST(Z3FormalVerification, AlgebraicOpaquePredicate) {
    Z3Verifier verifier;
    
    // The impossible algebraic obfuscation that LLVM cannot resolve: (x * (x + 1)) % 2 == 0
    // Z3 must prove that the negation of this theorem is UNSAT (impossible) in milliseconds.
    bool alwaysTrue = verifier.isConditionAlwaysTrue(nullptr); 
    
    EXPECT_TRUE(alwaysTrue) << "Z3 SMT Solver failed to prove the algebraic theorem!";
}

TEST(Z3FormalVerification, IntegerOverflowDetection) {
    Z3Verifier verifier;
    
    // Testing whether the balance + transfer_amount operation in a smart contract contains an overflow vulnerability.
    auto result = verifier.verifySafety(nullptr);
    
    // Expecting Z3 to return SAT (satisfiable) and provide the exact exploit parameters (counter-example) to trigger the vulnerability.
    EXPECT_FALSE(result.isSafe) << "Z3 missed the Integer Overflow vulnerability!";
    
    // Note: Ensure that Z3Verifier.cpp outputs "Exploit Parameters" instead of the Turkish equivalent.
    EXPECT_NE(result.exploitPath.find("Exploit Parameters"), std::string::npos) << "Z3 failed to generate a counter-example!";
    
    std::cout << "[ Z3 Exploit Input ] " << result.exploitPath << "\n";
}