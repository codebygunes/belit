# Belit Architecture Overview

Belit is engineered as a modular, high-performance verification and deobfuscation pipeline. The codebase is strictly decoupled into independent layers to maintain clean separation of concerns.

## Core Modules

[Raw Bytecode (EVM / Wasm)] 
          │
          ▼
   ┌──────────────┐
   │  Lifters     │ (EVMLifter, WasmLifter -> Generates Control Flow Graph / CFG)
   └──────┬───────┘
          │
          ▼
   ┌──────────────┐
   │ IR Translator│ (LLVMTranslator -> Lifts CFG into LLVM IR)
   └──────┬───────┘
          │
          ├────────────────────────┐
          ▼                        ▼
   ┌──────────────┐        ┌──────────────┐
   │ LLVM Passes  │        │ Z3 Engine    │ (Z3SymbolicEngine -> Mathematical SMT
   │ (Deobfuscate)│        │              │  Verification & Overflow/Underflow check)
   └──────────────┘        └──────────────┘

### Module Breakdown
* **`src/targets/`**: Translates raw architecture-specific bytecodes into Belit’s universal `BasicBlock` and `Instruction` intermediate representations.
* **`src/ir/`**: Handles LLVM IR generation (`LLVMTranslator`) for compiler optimization compatibility.
* **`src/llvm_passes/`**: Implements custom LLVM transform and deobfuscation passes (registered via `BelitPassPlugin`).
* **`src/verifier/`**: Houses the SMT-backed symbolic execution engine (`Z3SymbolicEngine`) using dual-layer BitVector validation.