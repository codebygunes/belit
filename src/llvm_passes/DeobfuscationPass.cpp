#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <z3++.h>
#include <map>

using namespace llvm;

namespace {
struct DeobfuscationPass : public PassInfoMixin<DeobfuscationPass> {
    
    // Engine to translate LLVM IR into a Z3 Mathematical Model (AST) on-the-fly
    // SOLUTION 1: Passing the Z3 Context by reference from the outside
    z3::expr translateToZ3(z3::context &ctx, Value *V, std::map<Value*, z3::expr> &symVars) {
        // SOLUTION 2: Using find() instead of operator[] to prevent Default Constructor errors
        auto it = symVars.find(V);
        if (it != symVars.end()) return it->second;
        
        if (auto *CI = dyn_cast<ConstantInt>(V)) {
            return ctx.bv_val(static_cast<uint64_t>(CI->getZExtValue()), 256u);
        }
        
        if (auto *Arg = dyn_cast<Argument>(V)) {
            z3::expr sym = ctx.bv_const(Arg->getName().str().c_str(), 256u);
            symVars.insert({V, sym});
            return sym;
        }
        
        if (auto *I = dyn_cast<Instruction>(V)) {
            if (I->getOpcode() == Instruction::Add) {
                return translateToZ3(ctx, I->getOperand(0), symVars) + translateToZ3(ctx, I->getOperand(1), symVars);
            } else if (I->getOpcode() == Instruction::Mul) {
                return translateToZ3(ctx, I->getOperand(0), symVars) * translateToZ3(ctx, I->getOperand(1), symVars);
            } else if (I->getOpcode() == Instruction::URem) {
                return z3::urem(translateToZ3(ctx, I->getOperand(0), symVars), translateToZ3(ctx, I->getOperand(1), symVars));
            }
        }
        
        // Fallback for unknown instructions
        z3::expr sym = ctx.bv_const("unknown", 256u);
        symVars.insert({V, sym});
        return sym;
    }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        bool Changed = false;
        
        // Z3-POWERED OPAQUE PREDICATE SOLVER
        for (BasicBlock &BB : F) {
            if (auto *BI = dyn_cast<BranchInst>(BB.getTerminator())) {
                if (BI->isConditional()) {
                    if (auto *Cmp = dyn_cast<ICmpInst>(BI->getCondition())) {
                        if (Cmp->getPredicate() == CmpInst::ICMP_EQ) {
                            try {
                                // SOLUTION 1 CONTINUED: Keeping Z3 objects at the function (local scope) level
                                // rather than class level to ensure the LLVM Pass remains portable.
                                z3::context ctx;
                                std::map<Value*, z3::expr> symVars;
                                
                                z3::expr lhs = translateToZ3(ctx, Cmp->getOperand(0), symVars);
                                z3::expr rhs = translateToZ3(ctx, Cmp->getOperand(1), symVars);
                                
                                z3::expr condition = (lhs == rhs);
                                
                                z3::solver solver(ctx);
                                // Asking Z3: "Is there any possibility that this condition is FALSE?"
                                solver.add(!condition);
                                
                                if (solver.check() == z3::unsat) {
                                    // Z3 proved it: Impossible! Therefore, this condition is always TRUE.
                                    BI->setCondition(ConstantInt::getTrue(Cmp->getContext()));
                                    Changed = true;
                                }
                            } catch (...) {
                                // Ignore translation errors to prevent compiler crashes.
                            }
                        }
                    }
                }
            }
        }

        // CFG Simplification
        for (BasicBlock &BB : F) {
            Changed |= ConstantFoldTerminator(&BB);
        }

        // Remove unreachable blocks orphaned by Z3's branch pruning
        Changed |= removeUnreachableBlocks(F);

        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
} // end anonymous namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "DeobfuscationPass", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "deobfuscate") {
                        FPM.addPass(DeobfuscationPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}