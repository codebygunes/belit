#include "belit/llvm_passes/DeobfuscationPass.hpp"
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm::PatternMatch;

namespace belit {
namespace passes {

llvm::PreservedAnalyses DeobfuscationPass::run(llvm::Module& M, llvm::ModuleAnalysisManager&) {
    bool changed = false;         
    
    // Iterate through all functions in the module to apply optimization and deobfuscation
    for (auto& F : M) {
        if (!F.isDeclaration()) {
            changed |= foldConstants(F);
            changed |= removeOpaquePredicates(F);
        }
    }         
    
    return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}

bool DeobfuscationPass::foldConstants(llvm::Function& F) {
    bool changed = false;
    
    // Arithmetic simplification and constant folding at the LLVM IR level
    for (auto& BB : F) {
        for (auto instIt = BB.begin(), instEnd = BB.end(); instIt != instEnd;) {
            llvm::Instruction& I = *instIt++;
            
            // Example: ADD(X, 0) -> X or SUB(X, 0) -> X transformations
            if (auto* BO = llvm::dyn_cast<llvm::BinaryOperator>(&I)) {
                llvm::Value* op1 = BO->getOperand(0);
                llvm::Value* op2 = BO->getOperand(1);
                
                if (BO->getOpcode() == llvm::Instruction::Add || BO->getOpcode() == llvm::Instruction::Sub) {
                    if (auto* C = llvm::dyn_cast<llvm::ConstantInt>(op2)) {
                        if (C->isZero()) {
                            BO->replaceAllUsesWith(op1);
                            BO->eraseFromParent();
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return changed;
}

bool DeobfuscationPass::removeOpaquePredicates(llvm::Function& F) {
    bool changed = false;
    
    // Cleanup of opaque conditions and dead basic blocks that are always true/false
    for (auto& BB : F) {
        auto* TI = BB.getTerminator();
        if (auto* BI = llvm::dyn_cast<llvm::BranchInst>(TI)) {
            if (BI->isConditional()) {
                if (auto* CI = llvm::dyn_cast<llvm::ConstantInt>(BI->getCondition())) {
                    // If the condition reduced to a constant value, flatten the control flow
                    llvm::BasicBlock* targetDest = CI->isOne() ? BI->getSuccessor(0) : BI->getSuccessor(1);
                    llvm::BasicBlock* deadDest = CI->isOne() ? BI->getSuccessor(1) : BI->getSuccessor(0);
                    
                    // Update references of the dead branch and make the jump unconditional
                    llvm::BranchInst::Create(targetDest, BI);
                    BI->eraseFromParent();
                    
                    // Mark basic block end optimization
                    deadDest->removePredecessor(&BB);
                    changed = true;
                }
            }
        }
    }
    return changed;
}

void runDeobfuscationPipeline(llvm::Module& M) {
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB;
    
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    llvm::ModulePassManager MPM;         
    
    // 1. Adding custom Belit Deobfuscation Pass
    MPM.addPass(DeobfuscationPass());         
    
    // 2. Standard O2 level optimizations to clean up remaining dead code
    MPM.addPass(PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2));
    
    MPM.run(M, MAM);
}

} // namespace passes
} // namespace belit