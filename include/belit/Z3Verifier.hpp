#pragma once
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <z3++.h>
#include <string>
#include <map>

namespace belit {
    struct VerificationResult {
        bool isSafe;
        std::string message;
        std::string exploitPath; 
    };

    class Z3Verifier {
    public:
        Z3Verifier();
        
        bool isConditionAlwaysTrue(llvm::Instruction* condInst);
        VerificationResult verifySafety(llvm::Module* module);
        
        // Crucial function allowing the E2E test to access the Z3 engine:
        z3::context& getContext() { return ctx; }

    private:
        z3::context ctx;
        z3::solver solver;
        std::map<llvm::Value*, z3::expr> symbolicVars;

        z3::expr getOrCreateExpr(llvm::Value* val);
    };
}