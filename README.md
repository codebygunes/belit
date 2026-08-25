# Belit: EVM LLVM Lifter & Z3 Formal Verification Engine

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![C++](https://img.shields.io/badge/Language-C++17-orange.svg)
![LLVM](https://img.shields.io/badge/Framework-LLVM_18-red.svg)
![Z3](https://img.shields.io/badge/Solver-Z3_SMT-yellow.svg)

Belit is a deep-tech compiler engineering tool designed to lift raw EVM (Ethereum Virtual Machine) bytecode into LLVM Intermediate Representation (IR), deobfuscate complex control flow mechanisms using custom LLVM passes, and mathematically prove the existence or absence of smart contract vulnerabilities (e.g., Re-entrancy and Integer Overflows) using the Microsoft Z3 SMT Solver.

Built as an open-source Public Good for the Ethereum ecosystem.

## Scope & Limitations (Alpha PoC Phase)
The current release of the Belit Engine operates as an **Alpha Proof-of-Concept (PoC)**. To ensure mathematical rigor in the Z3 formal verification pipeline, this phase explicitly limits the EVM opcode scope.

**Currently Supported:**
* Core stack operations and arithmetic/logic instructions (`PUSHx`, `ADD`, `SUB`, `MLOAD`, `MSTORE`).
* Dynamic control flow resolution (`JUMP`, `JUMPI`, `JUMPDEST`) using strictly LLVM `SwitchInst` and SSA mapping.
* Mathematical detection of Re-entrancy patterns and Integer Overflow vulnerabilities.
* Unimplemented opcodes trigger a *Graceful Degradation* state (returning 0) to maintain analysis pipeline integrity.

**Out of Scope for Alpha (Planned for Milestone 1):**
* Cross-contract interactions and external context switching (`CALL`, `DELEGATECALL`).
* Persistent state trie simulations (`SSTORE`/`SLOAD`) and Transient Storage (EIP-1153).

## Core Architecture
1. **EVM Lifter (`src/evm_lifter`):** Translates raw EVM bytecode into purely static Single Assignment (SSA) LLVM IR via a custom Two-Pass architecture, eliminating static mocking.
2. **Deobfuscator Pass (`src/llvm_passes`):** A custom LLVM 18 Pass Plugin that eliminates opaque predicates and hyperbolic control flow obfuscations before SMT analysis.
3. **Z3 Verifier (`src/smt_solver`):** Evaluates the normalized LLVM IR against strict mathematical constraints to detect exploits instantly and generate SAT counter-examples.

## Build Instructions

### Prerequisites
* CMake (>= 3.14)
* LLVM 18 (`llvm-18`, `llvm-18-dev`)
* Z3 SMT Solver (`libz3-dev`)
* Google Test (`libgtest-dev`)

### Linux / WSL (Ubuntu) Setup
```bash
sudo apt-get update
sudo apt-get install llvm-18 llvm-18-dev libz3-dev libgtest-dev cmake build-essential

git clone https://github.com/codebygunes/belit.git
cd belit
mkdir build && cd build
cmake .. -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
cmake --build .
```

## Usage (CLI)
Once built, you can run the Belit CLI engine on raw EVM bytecode files. The tool supports outputting the lifted LLVM IR alongside the Z3 verification report.

```bash
# Run the formal verification engine on a vulnerable contract
./belit_cli ../tests/bytecode_samples/the_dao_reentrancy.hex --dump-ir
```

## Running the Test Suite
The repository includes comprehensive End-to-End (E2E) and Unit tests validating the Lifter and Z3 Engine's accuracy.

```bash
cd build
./BelitTests
```