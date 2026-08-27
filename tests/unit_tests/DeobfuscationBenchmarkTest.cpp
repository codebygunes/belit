#include <gtest/gtest.h>
#include "belit/targets/EVMLifter.hpp"
#include "belit/targets/WasmLifter.hpp"
#include "belit/ir/LLVMTranslator.hpp"
#include "belit/llvm_passes/DeobfuscationPass.hpp"
#include "belit/verifier/Z3SymbolicEngine.hpp"
#include <chrono>

TEST(BenchmarkTest, MultiArchitectureLifterConsistency) {
    belit::EVMLifter evmLifter;
    belit::WasmLifter wasmLifter;

    std::vector<uint8_t> evmBytes = {0x60, 0x00, 0x03, 0xF3};
    
    // Eski 8-byte başlık yerine, CFG üretecek Code Section içeren geçerli Wasm dizisi
    std::vector<uint8_t> wasmBytes = {
        0x00, 0x61, 0x73, 0x6D, // Magic
        0x01, 0x00, 0x00, 0x00, // Version 1
        0x0A, 0x09, 0x01, 0x07, 0x00, 0x41, 0x2A, 0x41, 0x0A, 0x6A, 0x0B // Section 10 (Code)
    };

    EXPECT_TRUE(evmLifter.parse(evmBytes));
    EXPECT_TRUE(wasmLifter.parse(wasmBytes));

    EXPECT_EQ(evmLifter.getArchitectureName(), "Ethereum Virtual Machine (EVM)");
    EXPECT_EQ(wasmLifter.getArchitectureName(), "WebAssembly (WASM)");
}

TEST(BenchmarkTest, DeobfuscationPassOptimizationThroughput) {
    belit::EVMLifter lifter;

    std::vector<uint8_t> obfuscatedSequence = {
        0x60, 0x01, 0x60, 0x02, 0x01, // Arithmetic
        0x60, 0x00, 0x03,             // Underflow
        0x56, 0x5B, 0xF3              // Control Flow Jump
    };
    ASSERT_TRUE(lifter.parse(obfuscatedSequence));

    belit::LLVMTranslator translator;
    auto module = translator.translateToLLVM(lifter.getCFG(), "benchmark_deobfuscation_module");
    ASSERT_NE(module, nullptr);

    auto startTime = std::chrono::high_resolution_clock::now();
    belit::passes::runDeobfuscationPipeline(*module);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    // Deobfuscation pass must execute under 200ms (200000 microseconds)
    EXPECT_LT(duration, 200000);
}