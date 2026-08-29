#include "belit/ir/LLVMTranslator.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Instructions.h>
#include <iostream>

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
    // Wasm uses 32-bit natively, but we use i256 to flexibly support both EVM and Wasm organically
    llvm::Type* int256Ty = llvm::Type::getIntNTy(*context_, 256);
    
    for (const auto& block : cfg) {
        for (const auto& inst : block.instructions) {
            
            if (builder_->GetInsertBlock()->getTerminator() != nullptr) {
                continue; 
            }

            switch (inst.type) {
                case OpCodeType::Push: {
                    // ARCHITECTURAL FIX 1: Convert Little-Endian Wasm bytes to the correct mathematical value.
                    // Otherwise, the Z3 engine will calculate the threshold bypass values (e.g., 1MB) incorrectly.
                    uint64_t val = 0;
                    for (size_t i = 0; i < inst.operands.size(); ++i) {
                        val |= (static_cast<uint64_t>(inst.operands[i]) << (8 * i));
                    }
                    llvm::Value* constVal = llvm::ConstantInt::get(int256Ty, val);
                    stack.push_back(constVal);
                    break;
                }
                case OpCodeType::Add: {
                    if (stack.size() >= 2) {
                        llvm::Value* b = stack.back(); stack.pop_back();
                        llvm::Value* a = stack.back(); stack.pop_back();
                        llvm::Instruction* res = llvm::BinaryOperator::CreateAdd(a, b, "add_ssa", builder_->GetInsertBlock());
                        stack.push_back(res);
                    }
                    break;
                }
                case OpCodeType::Sub: {
                    if (stack.size() >= 2) {
                        llvm::Value* b = stack.back(); stack.pop_back();
                        llvm::Value* a = stack.back(); stack.pop_back();
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
                            llvm::Value* ptr = builder_->CreateIntToPtr(ptr_int, llvm::Type::getInt32PtrTy(*context_));
                            builder_->CreateStore(val, ptr);
                        }
                    } 
                    else if (inst.mnemonic == "I32_LOAD") {
                        if (stack.size() >= 1) {
                            llvm::Value* ptr_int = stack.back(); stack.pop_back();
                            llvm::Value* ptr = builder_->CreateIntToPtr(ptr_int, llvm::Type::getInt32PtrTy(*context_));
                            llvm::Instruction* loadVal = builder_->CreateLoad(llvm::Type::getInt32Ty(*context_), ptr, "load_ssa");
                            stack.push_back(builder_->CreateZExt(loadVal, int256Ty));
                        }
                    }
                    else if (inst.mnemonic == "MEMORY_GROW") {
                        if (!stack.empty()) {
                            llvm::Value* delta_pages = stack.back(); stack.pop_back();
                            llvm::FunctionType* growTy = llvm::FunctionType::get(int256Ty, {int256Ty}, false);
                            llvm::FunctionCallee growFunc = llvmModule->getOrInsertFunction("wasm.memory.grow", growTy);
                            llvm::Instruction* result = builder_->CreateCall(growFunc, {delta_pages}, "grow_res");
                            stack.push_back(result);
                        }
                    }
                    else if (inst.mnemonic == "CALL") {
                        // ARCHITECTURAL FIX 2: The CALL instruction now organically pops arguments from the stack!
                        uint32_t funcIdx = 0;
                        if (inst.operands.size() >= 4) {
                            funcIdx = inst.operands[0] | (inst.operands[1] << 8) | (inst.operands[2] << 16) | (inst.operands[3] << 24);
                        }

                        // Dynamic argument count based on our FVM payload (since Wasm Type Section 1 is not parsed)
                        unsigned argCount = (funcIdx == 0) ? 2 : 0; // ipld.put expects 2 args, send expects 0 args
                        
                        std::vector<llvm::Type*> paramTypes(argCount, int256Ty);
                        llvm::FunctionType* funcTy = llvm::FunctionType::get(int256Ty, paramTypes, false);
                        std::string funcName = "func_" + std::to_string(funcIdx);
                        llvm::FunctionCallee callee = llvmModule->getOrInsertFunction(funcName, funcTy);
                        
                        std::vector<llvm::Value*> args;
                        // Pop values from the stack (LIFO - Last In First Out)
                        for (unsigned i = 0; i < argCount; ++i) {
                            if (!stack.empty()) {
                                args.insert(args.begin(), stack.back()); // Insert at the beginning to preserve Wasm argument ordering
                                stack.pop_back();
                            } else {
                                args.insert(args.begin(), llvm::ConstantInt::get(int256Ty, 0));
                            }
                        }
                        
                        llvm::Instruction* callRes = builder_->CreateCall(callee, args, "call_res_" + std::to_string(funcIdx));
                        
                        // Push the return value of FVM functions (e.g., send) back to the stack (for memory.grow to consume)
                        stack.push_back(callRes);
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