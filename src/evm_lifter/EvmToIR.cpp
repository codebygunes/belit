#include "belit/EvmToIR.hpp"
#include <llvm/IR/Verifier.h>
#include <stdexcept>
#include <iostream>

using namespace belit;
using namespace llvm;

// EVM Opcodes
#define OP_ADD 0x01
#define OP_SUB 0x03
#define OP_MLOAD 0x51
#define OP_MSTORE 0x52
#define OP_JUMP 0x56
#define OP_JUMPI 0x57
#define OP_JUMPDEST 0x5b
#define OP_PUSH1 0x60
#define OP_PUSH32 0x7f

EvmToIR::EvmToIR(LLVMContext& context, Module& module) 
    : ctx(context), mod(module), builder(context) {}

void EvmToIR::push(Value* val) {
    evmStack.push_back(val);
}

Value* EvmToIR::pop() {
    if (evmStack.empty()) {
        // SOLUTION: Alpha PoC Graceful Degradation
        // If the stack underflows due to an unsupported opcode (DUP, SWAP, CALL, etc.),
        // return '0' to maintain pipeline integrity instead of crashing the CLI.
        return llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx, 256), 0);
    }
    Value* val = evmStack.back();
    evmStack.pop_back();
    return val;
}

Function* EvmToIR::liftBytecode(const std::vector<uint8_t>& bytecode, const std::string& functionName) {
    Type* i256Ty = Type::getIntNTy(ctx, 256);
    FunctionType* funcType = FunctionType::get(Type::getVoidTy(ctx), false);
    Function* evmFunc = Function::Create(funcType, Function::ExternalLinkage, functionName, mod);

    BasicBlock* entryBB = BasicBlock::Create(ctx, "entry", evmFunc);
    builder.SetInsertPoint(entryBB);

    Type* i8Ty = Type::getInt8Ty(ctx);
    evmMemory = builder.CreateAlloca(i8Ty, ConstantInt::get(Type::getInt64Ty(ctx), 4096), "evm_memory");

    discoverJumpDestinations(evmFunc, bytecode);
    translateOpcodes(evmFunc, bytecode);

    verifyFunction(*evmFunc);
    return evmFunc;
}

void EvmToIR::discoverJumpDestinations(Function* func, const std::vector<uint8_t>& bytecode) {
    jumpDestinations.clear();
    for (uint64_t pc = 0; pc < bytecode.size(); ++pc) {
        uint8_t op = bytecode[pc];
        
        if (op == OP_JUMPDEST) {
            // Record only valid JUMPDEST instructions (not payload bytes)
            BasicBlock* bb = BasicBlock::Create(ctx, "jumpdest_" + std::to_string(pc), func);
            jumpDestinations[pc] = bb;
        } else if (op >= OP_PUSH1 && op <= OP_PUSH32) {
            // Security: Prevent out-of-bounds read if PUSH payload exceeds bytecode size (Truncated Bytecode)
            uint64_t pushBytes = (op - OP_PUSH1 + 1);
            if (pc + pushBytes >= bytecode.size()) {
                pc = bytecode.size(); // Skip to the end for safe exit
            } else {
                pc += pushBytes; // Skip PUSH payload bytes so they are not interpreted as opcodes
            }
        }
    }
}

void EvmToIR::translateOpcodes(Function* func, const std::vector<uint8_t>& bytecode) {
    Type* i256Ty = Type::getIntNTy(ctx, 256);
    
    // Block simulating EVM's REVERT/INVALID state for invalid jump destinations
    BasicBlock* invalidJumpBB = BasicBlock::Create(ctx, "invalid_jump", func);
    IRBuilder<> fallbackBuilder(invalidJumpBB);
    fallbackBuilder.CreateRetVoid();

    for (uint64_t pc = 0; pc < bytecode.size(); ++pc) {
        uint8_t op = bytecode[pc];

        if (jumpDestinations.count(pc)) {
            if (!builder.GetInsertBlock()->getTerminator()) {
                builder.CreateBr(jumpDestinations[pc]);
            }
            builder.SetInsertPoint(jumpDestinations[pc]);
        }

        switch (op) {
            case OP_PUSH1: {
                // Prevent crash by pushing 0 if bytecode is truncated
                uint8_t val = (pc + 1 < bytecode.size()) ? bytecode[++pc] : 0;
                push(ConstantInt::get(i256Ty, val));
                break;
            }
            case OP_ADD: {
                Value* a = pop(); Value* b = pop();
                push(builder.CreateAdd(a, b, "add_res"));
                break;
            }
            case OP_SUB: {
                Value* a = pop(); Value* b = pop();
                push(builder.CreateSub(a, b, "sub_res"));
                break;
            }
            case OP_MSTORE: {
                Value* offset = pop(); Value* val = pop();    
                Value* ptr = builder.CreateGEP(Type::getInt8Ty(ctx), evmMemory, offset, "mstore_ptr");
                builder.CreateStore(val, ptr);
                break;
            }
            case OP_MLOAD: {
                Value* offset = pop();
                Value* ptr = builder.CreateGEP(Type::getInt8Ty(ctx), evmMemory, offset, "mload_ptr");
                Value* loadedVal = builder.CreateLoad(i256Ty, ptr, "mload_res");
                push(loadedVal);
                break;
            }
            case OP_JUMPDEST: {
                break;
            }
            case OP_JUMP: {
                Value* targetPC = pop();
                
                // Dynamic Switch: If the target PC is not in 'jumpDestinations', fallback to 'invalidJumpBB' (REVERT)!
                SwitchInst* switchInst = builder.CreateSwitch(targetPC, invalidJumpBB, jumpDestinations.size());
                for (auto const& [validPC, bb] : jumpDestinations) {
                    switchInst->addCase(cast<ConstantInt>(ConstantInt::get(i256Ty, validPC)), bb);
                }
                
                BasicBlock* deadBlock = BasicBlock::Create(ctx, "dead_code_after_jump", func);
                builder.SetInsertPoint(deadBlock);
                break;
            }
            case OP_JUMPI: {
                Value* targetPC = pop(); Value* condition = pop(); 
                Value* isTrue = builder.CreateICmpNE(condition, ConstantInt::get(i256Ty, 0), "jumpi_cond");
                
                BasicBlock* continueBB = BasicBlock::Create(ctx, "continue_" + std::to_string(pc+1), func);
                BasicBlock* jumpTargetBB = BasicBlock::Create(ctx, "dynamic_jumpi_target", func);

                builder.CreateCondBr(isTrue, jumpTargetBB, continueBB);

                builder.SetInsertPoint(jumpTargetBB);
                SwitchInst* switchInst = builder.CreateSwitch(targetPC, invalidJumpBB, jumpDestinations.size());
                for (auto const& [validPC, bb] : jumpDestinations) {
                    switchInst->addCase(cast<ConstantInt>(ConstantInt::get(i256Ty, validPC)), bb);
                }

                builder.SetInsertPoint(continueBB);
                break;
            }
            default:
                break;
        }
    }

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateRetVoid();
    }
}