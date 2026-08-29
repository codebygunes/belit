#include "belit/verifier/Z3SymbolicEngine.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallString.h>

namespace belit {

Z3SymbolicEngine::Z3SymbolicEngine() : context_(), solver_(context_) {}
Z3SymbolicEngine::~Z3SymbolicEngine() = default;

z3::expr Z3SymbolicEngine::getZ3Expr(llvm::Value* val, std::map<llvm::Value*, z3::expr>& env) {
    auto it = env.find(val);
    if (it != env.end()) {
        return it->second;
    }
    
    if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(val)) {
        llvm::SmallString<64> strVal;
        constInt->getValue().toStringUnsigned(strVal, 10);
        return context_.bv_val(strVal.c_str(), 32); 
    }
    
    return context_.bv_const(("sym_var_" + std::to_string(reinterpret_cast<uintptr_t>(val))).c_str(), 32);
}

VerificationResult Z3SymbolicEngine::verify(llvm::Module& module) {
    VerificationResult result;
    result.isSafe = true;
    std::map<llvm::Value*, z3::expr> sym_env;

    z3::sort address_sort = context_.bv_sort(32);
    z3::sort byte_sort = context_.bv_sort(8);
    z3::expr wasm_memory = context_.constant("wasm_memory", context_.array_sort(address_sort, byte_sort));
    z3::expr current_mem_size = context_.bv_val(65536, 32);

    for (auto& F : module) {
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (auto* castInst = llvm::dyn_cast<llvm::CastInst>(&I)) {
                    sym_env.insert({&I, getZ3Expr(castInst->getOperand(0), sym_env)});
                }
                else if (auto* binOp = llvm::dyn_cast<llvm::BinaryOperator>(&I)) {
                    z3::expr z3_a = getZ3Expr(binOp->getOperand(0), sym_env);
                    z3::expr z3_b = getZ3Expr(binOp->getOperand(1), sym_env);
                    
                    if (binOp->getOpcode() == llvm::Instruction::Add) {
                        z3::expr res = z3_a + z3_b;
                        sym_env.insert({&I, res});
                        
                        solver_.push();
                        solver_.add(z3::ult(res, z3_a));
                        if (solver_.check() == z3::sat) {
                            result.isSafe = false;
                            result.discoveredViolations.push_back("Mathematical Integer Overflow detected.");
                        }
                        solver_.pop();
                    } else if (binOp->getOpcode() == llvm::Instruction::Sub) {
                        z3::expr res = z3_a - z3_b;
                        sym_env.insert({&I, res});
                        
                        solver_.push();
                        solver_.add(z3::ult(z3_a, z3_b));
                        if (solver_.check() == z3::sat) {
                            result.isSafe = false;
                            result.discoveredViolations.push_back("Mathematical Integer Underflow detected.");
                        }
                        solver_.pop();
                    }
                }
                else if (auto* storeInst = llvm::dyn_cast<llvm::StoreInst>(&I)) {
                    z3::expr ptr_addr = getZ3Expr(storeInst->getPointerOperand(), sym_env);
                    z3::expr val = getZ3Expr(storeInst->getValueOperand(), sym_env);

                    solver_.push();
                    solver_.add(z3::uge(ptr_addr, current_mem_size));
                    if (solver_.check() == z3::sat) {
                        result.isSafe = false;
                        result.discoveredViolations.push_back("Out-of-Bounds Memory WRITE detected (Wasm Store Trap).");
                    }
                    solver_.pop();

                    wasm_memory = z3::store(wasm_memory, ptr_addr, val.extract(7, 0)); 
                }
                else if (auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(&I)) {
                    z3::expr ptr_addr = getZ3Expr(loadInst->getPointerOperand(), sym_env);

                    solver_.push();
                    solver_.add(z3::uge(ptr_addr, current_mem_size));
                    if (solver_.check() == z3::sat) {
                        result.isSafe = false;
                        result.discoveredViolations.push_back("Out-of-Bounds Memory READ detected (Wasm Load Trap).");
                    }
                    solver_.pop();

                    z3::expr loaded_byte = z3::select(wasm_memory, ptr_addr);
                    sym_env.insert({&I, z3::zext(loaded_byte, 24)}); 
                }
                // --- PROTOCOL LABS FVM EXCLUSIVE HOST CALL TRACKING ---
                else if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (callInst->getCalledFunction()) {
                        llvm::StringRef funcName = callInst->getCalledFunction()->getName();
                        
                        if (funcName == "wasm.memory.grow") {
                            z3::expr pages = getZ3Expr(callInst->getArgOperand(0), sym_env);
                            current_mem_size = current_mem_size + (pages * context_.bv_val(65536, 32));
                            sym_env.insert({&I, current_mem_size});
                        }
                        // Detect IPLD Block Manipulation (FVM fvm::ipld::put)
                        else if (funcName.starts_with("ipld::put") || funcName == "fvm.ipld.put") {
                            z3::expr size = getZ3Expr(callInst->getArgOperand(1), sym_env); // Size operand
                            
                            solver_.push();
                            // FVM enforces max IPLD block sizes. Check if symbolic bounds can exceed 1MB.
                            solver_.add(z3::ugt(size, context_.bv_val(1048576, 32))); 
                            if (solver_.check() == z3::sat) {
                                result.isSafe = false;
                                result.discoveredViolations.push_back("FVM IPLD State Manipulation: Attempted to write an unconstrained or oversized data block to IPLD state.");
                            }
                            solver_.pop();
                        }
                        // Detect Cross-Actor Reentrancy (FVM fvm::actor::send)
                        else if (funcName.starts_with("actor::send") || funcName == "fvm.send") {
                            result.isSafe = false;
                            result.discoveredViolations.push_back("FVM Actor Security Violation: Cross-actor 'send' call detected without provable state-lock validation (Reentrancy risk).");
                        }
                    }
                }
            }
        }
    }

    if (result.isSafe) {
        result.report = "Verification passed: Linear memory boundaries and FVM host constraints verified.";
    } else {
        result.report = "Verification failed: FVM State violations or mathematical traps detected.";
    }

    return result;
}

} // namespace belit