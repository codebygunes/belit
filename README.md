# Belit: Formal Verification Middleware for Wasm & Decentralized Compute

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![C++](https://img.shields.io/badge/Language-C++20-orange.svg)
![LLVM](https://img.shields.io/badge/Framework-LLVM_21-red.svg)
![Z3](https://img.shields.io/badge/Solver-Z3_SMT-yellow.svg)

Belit is an open-source compiler-infrastructure middleware designed to secure trustless execution nodes in decentralized networks. By lifting raw compiled bytecodes (specifically WebAssembly and EVM) into LLVM Intermediate Representation (IR), Belit bridges decentralized execution environments with the Microsoft Z3 SMT Solver.

Instead of relying on heuristic scanners, Belit applies formal mathematical constraints to deobfuscate complex control flows and mathematically prove the existence or absence of critical vulnerabilities (e.g., memory corruptions, integer overflows). It is built as a Public Good to enable bulletproof verifiable computation for Compute-over-Data infrastructures and decentralized sandboxes.

## Scope & Limitations (Alpha PoC Phase)
The current release of the Belit Engine operates as an Alpha Proof-of-Concept (PoC). To ensure absolute mathematical rigor in the Z3 formal verification pipeline, this phase explicitly limits the opcode scope.

**Currently Supported:**
* Core stack operations and arithmetic/logic instructions across Wasm and EVM.
* Dynamic control flow resolution using strictly LLVM SwitchInst and SSA mapping.
* Mathematical detection of state violations (Underflow/Overflow) in trustless payloads.
* Unimplemented opcodes trigger a Graceful Degradation state to maintain analysis pipeline integrity.

**Out of Scope for Alpha (Targeted for Next Milestones):**
* Persistent state modeling and dynamic Wasm linear memory (`memory.grow`) formalization.
* Cross-environment context switching and external calls within distributed nodes.

## Core Architecture
1. **Target-Agnostic Lifter (`src/targets`):** Translates raw Wasm and EVM bytecode into purely static Single Assignment (SSA) LLVM IR via a custom Two-Pass architecture, optimizing for decentralized VM compatibility.
2. **Deobfuscator Pass (`src/llvm_passes`):** A custom LLVM Pass Plugin that mathematically eliminates opaque predicates and flattened control flow graphs before SMT analysis, ensuring malicious payloads cannot hide state violations.
3. **Z3 Verifier (`src/verifier`):** Evaluates the normalized LLVM IR against strict bit-vector constraints to detect exploits instantly and generate SAT counter-examples for node operators.

## Ecosystem Integration & Adoption
Belit bridges seamlessly with modern verifiable compute ecosystems:
* **LLVM Toolchain Native Support:** Through `BelitPassPlugin`, any standard LLVM-based compiler pipeline or decentralized node can directly load Belit as a dynamic shared module (`-fload-pass-plugin=libBelitPassPlugin.so`) to enforce automated verification before execution.
* **Headless Node & CI/CD Native:** Designed to run headlessly as a GitHub Action or directly within distributed network nodes to block vulnerable executions mathematically.

## Build Instructions

Prerequisites:
* CMake (>= 3.20)
* LLVM 21 (llvm-21, llvm-21-dev)
* Z3 SMT Solver (libz3-dev)
* Google Test (libgtest-dev)

Linux / WSL (Ubuntu) Setup:
```bash
sudo apt-get update
sudo apt-get install llvm-21 llvm-21-dev libz3-dev libgtest-dev cmake build-essential

git clone https://github.com/codebygunes/belit.git
cd belit
cmake -B build
cmake --build build -j 4