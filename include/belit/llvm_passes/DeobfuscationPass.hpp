#pragma once

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>

namespace belit {
namespace passes {

// Modern LLVM Pass definition for Control Flow Deobfuscation
class DeobfuscationPass : public llvm::PassInfoMixin<DeobfuscationPass> {
public:
    llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM);
    
    // Required by new LLVM PassManager
    static bool isRequired() { return true; }

private:
    bool removeOpaquePredicates(llvm::Function& F);
    bool foldConstants(llvm::Function& F);
};

// Helper interface to run the complete optimization and deobfuscation pipeline
void runDeobfuscationPipeline(llvm::Module& M);

} // namespace passes
} // namespace belit