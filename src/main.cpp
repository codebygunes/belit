#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "belit/targets/WasmLifter.hpp"
#include "belit/ir/LLVMTranslator.hpp"
#include "belit/llvm_passes/DeobfuscationPass.hpp"
#include "belit/verifier/Z3SymbolicEngine.hpp"

namespace {

void printBanner() {
    std::cout << "=========================================================\n";
    std::cout << "  BELIT: Formal Verification & FVM Security Engine       \n";
    std::cout << "  Protocol Labs Public Good Infrastructure               \n";
    std::cout << "=========================================================\n\n";
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <payload.wasm> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --dump-ir       Output the standardized LLVM IR to console.\n";
    std::cout << "  --skip-opt      Skip optimization and deobfuscation passes.\n";
    std::cout << "  -h, --help      Display this help message and exit.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    printBanner();

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string filePath;
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
            if (filePath.empty()) {
                filePath = arg;
            } else {
                std::cerr << "[ERROR] Multiple file paths provided: " << arg << "\n";
                return 1;
            }
        }
    }

    if (filePath.empty()) {
        std::cerr << "[ERROR] Target Wasm or binary payload path is required.\n";
        return 1;
    }

    std::cout << "[*] Loading target bytecode from: " << filePath << "...\n";
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[CRITICAL] Failed to open payload file on disk.\n";
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytecode(size);
    if (!file.read(reinterpret_cast<char*>(bytecode.data()), size)) {
        std::cerr << "[CRITICAL] Failed to read bytecode stream.\n";
        return 1;
    }

    // Step 1: Secure Lifting (Strict Halt & Prove Model)
    std::cout << "[1/4] Parsing Wasm Binary & Section Decoding...\n";
    belit::WasmLifter lifter;
    if (!lifter.parse(bytecode)) {
        std::cerr << "[VERIFICATION FAILED] Lifter rejected payload due to strict security bounds or unmapped opcodes.\n";
        return 1;
    }
    std::cout << "      -> CFG successfully generated. Basic blocks: " << lifter.getCFG().size() << "\n\n";

    // Step 2: LLVM IR Translation (SSA Form State Modeling)
    std::cout << "[2/4] Translating CFG to Standardized LLVM IR...\n";
    belit::LLVMTranslator translator;
    auto module = translator.translateToLLVM(lifter.getCFG(), "fvm_verified_payload");
    if (!module) {
        std::cerr << "[CRITICAL] LLVM IR translation layer failed.\n";
        return 1;
    }

    // Symbol Linking for FVM Imports
    const auto& imports = lifter.getImportedFunctions();
    int importIdx = 0;
    for (auto& F : *module) {
        if (F.isDeclaration() && importIdx < imports.size()) {
            F.setName(imports[importIdx]);
            importIdx++;
        }
    }
    std::cout << "      -> LLVM Module initialized and symbols linked.\n\n";

    // Step 3: Deobfuscation & Optimization Pipeline
    if (!skipOpt) {
        std::cout << "[3/4] Running Deobfuscation and Constant Folding Passes...\n";
        llvm::ModuleAnalysisManager mam;
        belit::passes::DeobfuscationPass deobfuscator;
        deobfuscator.run(*module, mam);
        std::cout << "      -> Opaque predicates and dead blocks eliminated.\n\n";
    } else {
        std::cout << "[3/4] Skipping Deobfuscation Pipeline (--skip-opt enabled).\n\n";
    }

    if (dumpIr) {
        std::cout << "--- STANDARDIZED LLVM IR DUMP ---\n";
        translator.dumpIR(*module);
        std::cout << "--------------------------------\n\n";
    }

    // Step 4: Formal Verification via Z3 SMT Solver
    std::cout << "[4/4] Executing Z3 Symbolic Formal Verification Bridge...\n";
    belit::Z3SymbolicEngine verifier;
    belit::VerificationResult result = verifier.verify(*module);

    std::cout << "\n=========================================================\n";
    std::cout << "  VERIFICATION REPORT RESULT: " << (result.isSafe ? "[SAFE / VERIFIED]" : "[UNSAFE / VULNERABLE]") << "\n";
    std::cout << "=========================================================\n";
    std::cout << "Summary: " << result.report << "\n";

    if (!result.isSafe) {
        std::cout << "\nDiscovered Security Violations:\n";
        for (const auto& violation : result.discoveredViolations) {
            std::cout << " [X] " << violation << "\n";
        }
        std::cout << "\n[!] Action Required: Payload blocked from execution node.\n";
        return 2; // Exit code 2 indicates formal verification failure
    }

    std::cout << "\n[+] Success: Payload verified mathematically safe against FVM constraints.\n";
    return 0;
}