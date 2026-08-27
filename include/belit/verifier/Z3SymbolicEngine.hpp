#ifndef BELIT_Z3SYMBOLICENGINE_HPP
#define BELIT_Z3SYMBOLICENGINE_HPP

#include <vector>
#include <string>
#include <map>
#include <z3++.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include "belit/core/Common.hpp"

namespace belit {

class Z3SymbolicEngine {
public:
    Z3SymbolicEngine();
    ~Z3SymbolicEngine();
    
    // Directly accepts the LLVM Module to evaluate deobfuscated SSA form
    VerificationResult verify(llvm::Module& module);

private:
    z3::context context_;
    z3::solver solver_;
    
    // Helper to map LLVM SSA values to Z3 Symbolic Expressions
    z3::expr getZ3Expr(llvm::Value* val, std::map<llvm::Value*, z3::expr>& env);
};

} // namespace belit

#endif // BELIT_Z3SYMBOLICENGINE_HPP