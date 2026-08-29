# Protocol Labs Grant Proposal: Belit - Formal Verification Engine for FVM

## 1. Project Overview
**Project Name:** Belit  
**Category:** Core Infrastructure / WebAssembly & Filecoin Virtual Machine (FVM) Security  
**License:** MIT (Public Good)

### 1.1. Abstract
In the Filecoin Virtual Machine (FVM) and decentralized Compute-over-Data infrastructures, trusting execution nodes with WebAssembly (Wasm) payloads is a critical bottleneck. Current security paradigms heavily rely on heuristic static analysis tools, pattern matchers, and fuzzers. While effective for common anti-patterns, heuristics guess rather than prove; they cannot mathematically guarantee the absence of complex state manipulation, memory leaks, or obfuscated vulnerabilities at the virtual machine bytecode level.

**Belit** eliminates this uncertainty by introducing a unique architectural paradigm: it is an open-source SMT-LLVM bridge that translates FVM WebAssembly binary targets into a single, standardized Static Single Assignment (SSA) LLVM Intermediate Representation (IR). Going beyond generic Wasm analysis, Belit explicitly models FVM host environment constraints. By combining custom LLVM deobfuscation pass plugins with the Microsoft Z3 SMT solver, Belit models software execution including IPLD state writes and cross-actor messages as rigorous geometric and bit-vector constraints. 

We have developed a **Functional Alpha Proof-of-Concept (PoC)** that successfully parses production-grade Wasm bytecodes, decodes LEB128 variables, processes Section 10 (Code) structured control flows, and translates memory operations directly into the LLVM IR bridge. Crucially, Belit enforces a **"Strict Halt & Prove"** model: any unmapped opcode or structural corruption immediately halts execution and rejects the payload, preventing bypass exploits. 

This grant will fund the evolution of Belit into a robust, developer-friendly CLI tool (`belit analyze`) and headless CI/CD security infrastructure, optimized specifically for Protocol Labs' FVM and distributed nodes.

### 1.2. Problem Statement & "Verifiable Computation"
While binary analysis tools exist, decentralized network operators and FVM smart contract developers face critical bottlenecks that existing tools fail to solve:
1. **The Heuristic Gap in Trustless Nodes:** Fuzzers provide a probability of security, not a mathematical proof. In FVM, state manipulations require absolute certainty.
2. **Blindness to FVM Host Constraints:** Standard analyzers are unaware of Protocol Labs' specific state environment. They cannot mathematically bound an `ipld::put` operation to prevent storage bloat, nor track the state-lock validations required before a cross-actor `fvm::send`.
3. **Fragmented Tooling Ecosystem:** There is no universal, accessible open-source bridge translating Wasm bytecodes natively into formal verification solvers without massive manual overhead.
4. **Obfuscation Resistance:** Malicious actors leverage opaque predicates and flattened control flow graphs to defeat standard inspection and bypass gas metering.
5. **Zero-Tolerance for Unknowns:** Traditional tools often attempt to "gracefully degrade" upon encountering unknown opcodes. Belit enforces a strict halt model where unmapped opcodes are instantly rejected.

Belit is a **compiler-infrastructure verification middleware**. By bridging low-level bytecodes directly into compiler-native LLVM IR and mapping SSA values to Z3 SMT constraints, Belit enforces automated, bulletproof mathematical proofs for sandboxed FVM execution.

### 1.3. Value Proposition, Ecosystem Benefit & Developer Experience
Belit acts as an essential "Public Good" infrastructure for the Protocol Labs ecosystem:
* **Frictionless Developer Experience & Zero-Setup:** To mitigate adoption friction regarding heavy C++ dependencies (LLVM/Z3), Belit provides pre-compiled containerized distributions (`Docker`) and a ready-to-use GitHub Action (`belit-action`). Developers and node operators interact with the engine via a single command or automated CI pipeline without compiling toolchains locally.
* **For FVM Node Operators:** A definitive logical proof engine that replaces guesswork with mathematical certainty, actively preventing out-of-bounds memory traps and malicious IPLD storage bloat before untrusted payloads execute.
* **For Smart Contract Developers:** A lightweight CLI pipeline to catch impossible state transitions, cross-actor reentrancy risks, integer overflows, and underflows locally during the development lifecycle.

---

## 2. Team Background, Feasibility & Sustainability Mitigations

### 2.1. Lead Compiler Engineer
**Lead Compiler Engineer:** Elif Nur Ayhan (@codebygunes)  
As a Software Engineer specializing in deep-tech security, formal verification, and LLVM-based transformations, I have single-handedly developed the current Belit Alpha. The PoC already features fully functional native Wasm bytecode parsing, strict unmapped opcode halting, SSA stack modeling, and active Z3 SMT integration via LLVM IR bridging. 

### 2.2. Addressing Committee Risks & Sustainability
To ensure operational transparency and long-term viability, we address potential committee concerns proactively:
* **Key-Person Dependency & Community Roadmap:** The project is structured around a highly modular, clean-room compiler architecture (`ILifter`, `LLVMTranslator`, `Z3SymbolicEngine`). Comprehensive unit and E2E test suites paired with automated GitHub Actions CI/CD pipelines ensure seamless community audits. Furthermore, we commit to opening an early-access pilot program with volunteer FVM node operators and smart contract developers during Milestone 3 for real-world feedback loops.
* **Realistic Phased Rollout & Scope Management:** To prevent state explosion risks within the 12-week window, the Z3 solver integration employs a tiered validation strategy (prioritizing critical memory bounds and IPLD limits first) rather than unbounded state space exploration.
* **Post-Grant Sustainability:** As a pure MIT-licensed Public Good, Belit relies on standardized compiler building blocks (LLVM/Z3) that naturally align with broader open-source tooling, ensuring it remains maintainable and adoptable by any FVM-focused node infrastructure.

---

## 3. Budget & Funding Request ($45,000 USDC / 12 Weeks)

| Milestone | Duration | Focus Area | Requested Grant Amount |
| :--- | :--- | :--- | :--- |
| **Milestone 1** | Weeks 1 - 4 | **FVM Linear Memory Modeling:** Expanding the existing `WasmLifter` to strictly map WebAssembly dynamic memory operations (`memory.grow`, `memory.size`, `i32.store`) into exact Z3 BitVector boundaries to mathematically prove the absence of out-of-bounds writes in FVM payloads. | $15,000 USDC |
| **Milestone 2** | Weeks 5 - 8 | **IPLD Limits & Deobfuscation Passes:** Optimizing the LLVM `DeobfuscationPass` to eliminate control flow flattening, while formally modeling FVM syscalls (`ipld::put`, `send`). This maps cleaned SSA forms to Z3 to prove strict constraints on IPLD block sizes and prevent cross-actor reentrancy. | $15,000 USDC |
| **Milestone 3** | Weeks 9 - 12 | **FVM Ecosystem Integration, Docker & Pilot Testing:** Delivering a fully automated verification pipeline ready for headless FVM node integration, packaged with Docker images, GitHub Actions, and evaluated through an early-access pilot feedback program with community developers. | $15,000 USDC |
| **TOTAL** | **12 Weeks** | **End-to-End FVM Verification Infrastructure** | **$45,000 USDC** |