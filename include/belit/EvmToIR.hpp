#pragma once
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <vector>
#include <map>
#include <string>

namespace belit {

class EvmToIR {
public:
    EvmToIR(llvm::LLVMContext& context, llvm::Module& module);
    
    // Lifts raw EVM hex or byte array into LLVM IR
    llvm::Function* liftBytecode(const std::vector<uint8_t>& bytecode, const std::string& functionName = "evm_main");

private:
    llvm::LLVMContext& ctx;
    llvm::Module& mod;
    llvm::IRBuilder<> builder;

    // EVM Architectural State
    std::vector<llvm::Value*> evmStack;
    llvm::Value* evmMemory; // EVM Memory (Simulated via Alloca)
    
    // PC (Program Counter) -> LLVM BasicBlock Mapping
    std::map<uint64_t, llvm::BasicBlock*> jumpDestinations;
    
    // Helper Functions
    void push(llvm::Value* val);
    llvm::Value* pop();
    
    // Two-Pass Architecture
    void discoverJumpDestinations(llvm::Function* func, const std::vector<uint8_t>& bytecode);
    void translateOpcodes(llvm::Function* func, const std::vector<uint8_t>& bytecode);
};

} // namespace belit