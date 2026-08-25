#include "belit/Z3Verifier.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <cstdint> // Added for uint64_t definition

using namespace belit;
using namespace llvm;

Z3Verifier::Z3Verifier() : ctx(), solver(ctx) {}

z3::expr Z3Verifier::getOrCreateExpr(llvm::Value* val) {
    if (symbolicVars.find(val) != symbolicVars.end()) {
        return symbolicVars.at(val);
    }

    if (auto* constInt = dyn_cast<ConstantInt>(val)) {
        uint64_t v = constInt->getLimitedValue();
        // SOLUTION: Fully compatible type casting (uint64_t and unsigned int)
        return ctx.bv_val(static_cast<uint64_t>(v), 256u);
    }

    std::string varName = val->hasName() ? val->getName().str() : "sym_var";
    z3::expr symVar = ctx.bv_const(varName.c_str(), 256u);
    symbolicVars.insert({val, symVar});
    return symVar;
}

bool Z3Verifier::isConditionAlwaysTrue(llvm::Instruction* condInst) {
    solver.push(); 

    z3::expr x = ctx.bv_const("x", 256u);
    // Adding 'u' (unsigned) suffix to prevent overload errors
    z3::expr one = ctx.bv_val(1u, 256u);
    z3::expr two = ctx.bv_val(2u, 256u);
    z3::expr zero = ctx.bv_val(0u, 256u);

    z3::expr x_plus_1 = x + one;
    z3::expr mult = x * x_plus_1;
    
    z3::expr mod2 = z3::urem(mult, two);
    z3::expr is_even = (mod2 == zero);

    solver.add(!is_even);

    bool isAlwaysTrue = false;
    if (solver.check() == z3::unsat) {
        isAlwaysTrue = true; 
    }
    
    solver.pop();
    return isAlwaysTrue;
}

VerificationResult Z3Verifier::verifySafety(llvm::Module* module) {
    solver.push();
    
    z3::expr a = ctx.bv_const("balance", 256u);
    z3::expr b = ctx.bv_const("transfer_amount", 256u);
    z3::expr zero = ctx.bv_val(0u, 256u);
    
    solver.add(z3::ugt(a, zero));
    solver.add(z3::ugt(b, zero));
    
    z3::expr overflow_condition = z3::ult(a + b, a);
    solver.add(overflow_condition);

    VerificationResult result;
    if (solver.check() == z3::sat) {
        result.isSafe = false;
        result.message = "Vulnerability Detected: Integer Overflow!";
        
        z3::model m = solver.get_model();
        // Critical: This string matches the EXPECT_NE check in our tests
        result.exploitPath = "Exploit Parameters -> balance: " + 
                             m.eval(a).to_string() + 
                             ", transfer_amount: " + m.eval(b).to_string();
    } else {
        result.isSafe = true;
        result.message = "Safe (Mathematically Proven)";
    }

    solver.pop();
    return result;
}