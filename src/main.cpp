#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include "belit/targets/EVMLifter.hpp"
#include "belit/ir/LLVMTranslator.hpp"
#include "belit/llvm_passes/DeobfuscationPass.hpp"
#include "belit/verifier/Z3SymbolicEngine.hpp"

std::vector<uint8_t> parseHexFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open file: " << filepath << "\n";
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> bytes;
    std::string hexStr;

    for (char c : content) {
        if (std::isxdigit(c)) {
            hexStr += c;
        }
    }
    if (hexStr.length() % 2 != 0) {
        hexStr.pop_back();
    }
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        bytes.push_back(static_cast<uint8_t>(std::stoul(hexStr.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <path_to_bytecode.hex> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --dump-ir       Output the optimized LLVM IR to the console.\n";
    std::cout << "  --skip-opt      Skip LLVM Deobfuscation passes before verification.\n";
    std::cout << "  -h, --help      Show this help message.\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================\n";
    std::cout << "  BELIT: Formal Verification & Deobfuscation Engine\n";
    std::cout << "=========================================================\n\n";

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string filepath;
    bool dumpIr = false;
    bool skipOpt = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-ir") {
            dumpIr = true;
        } else if (arg == "--skip-opt") {
            skipOpt = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            if (filepath.empty()) {
                filepath = arg;
            } else {
                std::cerr << "[ERROR] Multiple file paths provided: " << arg << "\n";
                return 1;
            }
        }
    }

    if (filepath.empty()) {
        std::cerr << "[ERROR] Bytecode file path is required.\n";
        return 1;
    }

    std::cout << "[*] Loading bytecode from: " << filepath << "...\n";
    std::vector<uint8_t> targetBytecode = parseHexFile(filepath);
    
    if (targetBytecode.empty()) {
        std::cerr << "[ERROR] No valid bytecode found.\n";
        return 1;
    }

    // Step 1: Bytecode Parsing
    std::cout << "[1/4] Parsing Target Bytecode (EVM Lifter Alpha)...\n";
    belit::EVMLifter lifter;
    if (!lifter.parse(targetBytecode)) {
        std::cerr << "[ERROR] Failed to parse bytecode!\n";
        return 1;
    }
    std::cout << "      -> CFG successfully generated. Block count: " << lifter.getCFG().size() << "\n\n";

    // Step 2: LLVM IR Translation (Mandatory for the new Z3 Bridge)
    std::cout << "[2/4] Translating to LLVM IR (SSA Form State Modeling)...\n";
    belit::LLVMTranslator translator;
    auto llvmModule = translator.translateToLLVM(lifter.getCFG(), "belit_core_module");
    std::cout << "      -> Initial LLVM IR module created.\n\n";

    // Step 3: Deobfuscation (Optional based on flags, but feeds into Z3)
    if (!skipOpt) {
        std::cout << "[3/4] Running LLVM Deobfuscation and Optimization Pipeline...\n";
        belit::passes::runDeobfuscationPipeline(*llvmModule);
        std::cout << "      -> Opaque predicates removed and control flow simplified.\n\n";
    } else {
        std::cout << "[3/4] Skipping Deobfuscation Pipeline (--skip-opt flag enabled).\n\n";
    }

    if (dumpIr) {
        std::cout << "--- LLVM IR OUTPUT ---\n";
        translator.dumpIR(*llvmModule);
        std::cout << "----------------------\n\n";
    }

    // Step 4: Z3 SMT Verification (Now reading directly from LLVM Module)
    std::cout << "[4/4] Running Symbolic Formal Verification via Z3-LLVM Bridge...\n";
    belit::Z3SymbolicEngine verifier;
    auto verificationResult = verifier.verify(*llvmModule);

    std::cout << "Verification Result:\n";
    std::cout << "Status : " << (verificationResult.isSafe ? "[SAFE]" : "[VULNERABLE / UNSAFE]") << "\n";
    std::cout << "Report : " << verificationResult.report << "\n";

    if (!verificationResult.isSafe) {
        std::cout << "\nDetected Vulnerability Details:\n";
        for (const auto& violation : verificationResult.discoveredViolations) {
            std::cout << " - " << violation << "\n";
        }
    }

    std::cout << "\nExecution completed successfully.\n";
    return 0;
}