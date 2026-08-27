#ifndef BELIT_LLVMTRANSLATOR_HPP
#define BELIT_LLVMTRANSLATOR_HPP

#include <memory>
#include <vector>
#include <string>
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "belit/core/Common.hpp"

namespace belit {
class LLVMTranslator {
public:
    LLVMTranslator();
    ~LLVMTranslator();

    std::unique_ptr<llvm::Module> translateToLLVM(const std::vector<BasicBlock>& cfg, const std::string& moduleName);
    void dumpIR(const llvm::Module& module) const;

private:
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
};
} // namespace belit

#endif // BELIT_LLVMTRANSLATOR_HPP