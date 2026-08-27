#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "belit/llvm_passes/DeobfuscationPass.hpp"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "BelitDeobfuscationPlugin",
        "v1.0.0",
        [](PassBuilder &PB) {
            // Registering as a ModulePassManager (strict type requirement for global deobfuscation)
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "belit-deobfuscate") {
                        MPM.addPass(belit::passes::DeobfuscationPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}