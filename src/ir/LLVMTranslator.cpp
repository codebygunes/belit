#include "belit/ir/LLVMTranslator.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Instructions.h> // ADDED: For low-level Instruction classes

namespace belit {

LLVMTranslator::LLVMTranslator() {
    context_ = std::make_unique<llvm::LLVMContext>();
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

LLVMTranslator::~LLVMTranslator() = default;

std::unique_ptr<llvm::Module> LLVMTranslator::translateToLLVM(const std::vector<BasicBlock>& cfg, const std::string& moduleName) {
    auto llvmModule = std::make_unique<llvm::Module>(moduleName, *context_);
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder_->getVoidTy(), false);
    llvm::Function* mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "execute", llvmModule.get());
    
    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(*context_, "entry", mainFunc);
    builder_->SetInsertPoint(entryBB);
    
    std::vector<llvm::Value*> stack;
    llvm::Type* int256Ty = llvm::Type::getIntNTy(*context_, 256);
    
    for (const auto& block : cfg) {
        for (const auto& inst : block.instructions) {
            
            // FIX 2: Resolves the multiple "ret void" error. 
            // If the block is already terminated by a return/revert, stop appending new instructions to it.
            if (builder_->GetInsertBlock()->getTerminator() != nullptr) {
                continue; 
            }

            switch (inst.type) {
                case OpCodeType::Push: {
                    std::string hexStr;
                    for (uint8_t byte : inst.operands) {
                        char buf[3];
                        snprintf(buf, sizeof(buf), "%02x", byte);
                        hexStr += buf;
                    }
                    if (hexStr.empty()) hexStr = "0";
                    
                    llvm::APInt val(256, hexStr, 16);
                    llvm::Value* constVal = llvm::ConstantInt::get(int256Ty, val);
                    stack.push_back(constVal);
                    break;
                }
                case OpCodeType::Add: {
                    if (stack.size() >= 2) {
                        llvm::Value* b = stack.back(); stack.pop_back();
                        llvm::Value* a = stack.back(); stack.pop_back();
                        
                        // FIX 1: Bypassing IRBuilder's Constant Folding feature.
                        // Instructions will be forcefully written into the LLVM IR.
                        llvm::Instruction* res = llvm::BinaryOperator::CreateAdd(a, b, "add_ssa", builder_->GetInsertBlock());
                        stack.push_back(res);
                    }
                    break;
                }
                case OpCodeType::Sub: {
                    if (stack.size() >= 2) {
                        llvm::Value* b = stack.back(); stack.pop_back();
                        llvm::Value* a = stack.back(); stack.pop_back();
                        
                        // FIX 1: Bypassing IRBuilder's Constant Folding feature.
                        llvm::Instruction* res = llvm::BinaryOperator::CreateSub(a, b, "sub_ssa", builder_->GetInsertBlock());
                        stack.push_back(res);
                    }
                    break;
                }
                case OpCodeType::Unknown: {
                    if (inst.mnemonic == "I32_STORE") {
                        if (stack.size() >= 2) {
                            llvm::Value* val = stack.back(); stack.pop_back();
                            llvm::Value* ptr_int = stack.back(); stack.pop_back();
                            
                            // Convert integer to LLVM pointer
                            llvm::Value* ptr = builder_->CreateIntToPtr(ptr_int, llvm::Type::getInt32PtrTy(*context_));
                            builder_->CreateStore(val, ptr);
                        }
                    } 
                    else if (inst.mnemonic == "I32_LOAD") {
                        if (stack.size() >= 1) {
                            llvm::Value* ptr_int = stack.back(); stack.pop_back();
                            
                            llvm::Value* ptr = builder_->CreateIntToPtr(ptr_int, llvm::Type::getInt32PtrTy(*context_));
                            llvm::Instruction* loadVal = builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), ptr, "load_ssa");
                            stack.push_back(loadVal);
                        }
                    }
                    else if (inst.mnemonic == "MEMORY_GROW") {
                        if (stack.size() >= 1) {
                            llvm::Value* delta_pages = stack.back(); stack.pop_back();
                            
                            // Create dummy function signature for wasm.memory.grow
                            llvm::FunctionType* growTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context_), {llvm::Type::getInt32Ty(*context_)}, false);
                            llvm::FunctionCallee growFunc = llvmModule->getOrInsertFunction("wasm.memory.grow", growTy);
                            
                            llvm::Instruction* result = builder_->CreateCall(growFunc, {delta_pages}, "grow_res");
                            stack.push_back(result);
                        }
                    }
                    break;
                }
                case OpCodeType::Return:
                case OpCodeType::Revert: {
                    builder_->CreateRetVoid();
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (entryBB->getTerminator() == nullptr) {
        builder_->CreateRetVoid();
    }

    llvm::verifyFunction(*mainFunc);
    return llvmModule;
}

void LLVMTranslator::dumpIR(const llvm::Module& module) const {
    module.print(llvm::outs(), nullptr);
}

} // namespace belit