# Protocol Labs Grant Proposal: Belit - Formal Verification Engine for FVM

## 1. Project Overview
**Project Name:** Belit
**Category:** Core Infrastructure / WebAssembly & Filecoin Virtual Machine (FVM) Security
**License:** MIT (Public Good)

### 1.1. Abstract
In the Filecoin Virtual Machine (FVM) and decentralized Compute-over-Data infrastructures, trusting execution nodes with WebAssembly (Wasm) payloads is a critical bottleneck. Current security paradigms heavily rely on heuristic static analysis tools, pattern matchers, and fuzzers. While effective for common anti-patterns, heuristics guess rather than prove; they cannot mathematically guarantee the absence of complex state manipulation, memory leaks, or obfuscated vulnerabilities at the virtual machine bytecode level.

**Belit** eliminates this uncertainty by introducing a unique architectural paradigm: it is an open-source SMT-LLVM bridge that translates FVM WebAssembly binary targets into a single, standardized Static Single Assignment (SSA) LLVM Intermediate Representation (IR). By combining custom LLVM deobfuscation pass plugins with the Microsoft Z3 SMT solver, Belit models software execution as rigorous geometric and bit-vector constraints. 

We have developed a **Functional Alpha Proof-of-Concept (PoC)** that successfully parses production-grade Wasm bytecodes, decoding LEB128 variables, processing Section 10 (Code) structured control flows, and translating memory operations directly into the LLVM IR bridge. This grant will fund the evolution of Belit into a robust, production-grade automated CI/CD security infrastructure tool, optimized specifically for Protocol Labs' FVM and distributed nodes.

### 1.2. Problem Statement & "Verifiable Computation"
While binary analysis tools exist, decentralized network operators and FVM smart contract developers face critical bottlenecks that existing tools fail to solve:
1. **The Heuristic Gap in Trustless Nodes:** Fuzzers provide a probability of security, not a mathematical proof. In FVM, state manipulations require absolute certainty.
2. **Fragmented Tooling Ecosystem:** There is no universal, accessible open-source bridge translating Wasm bytecodes natively into formal verification solvers without massive manual overhead.
3. **Obfuscation Resistance:** Malicious actors leverage opaque predicates and flattened control flow graphs to defeat standard inspection. 
4. **Zero-Tolerance for Unknowns:** Traditional tools often attempt to "gracefully degrade" upon encountering unknown opcodes. Belit enforces a **Strict Halt & Prove** model—any unmapped opcode immediately flags the payload as unverifiable, eliminating bypass exploits.

Belit is a **compiler-infrastructure verification middleware**. By bridging low-level bytecodes directly into compiler-native LLVM IR and mapping SSA values to Z3 SMT constraints, Belit enforces automated, bulletproof mathematical proofs for sandboxed FVM execution.

### 1.3. Value Proposition / Ecosystem Benefit
Belit acts as an essential "Public Good" infrastructure for the Protocol Labs ecosystem:
* **For FVM Node Operators:** A definitive logical proof engine that replaces guesswork with mathematical certainty for untrusted Wasm payloads before they manipulate state.
* **For Smart Contract Developers:** A lightweight CLI pipeline (`belit`) to catch impossible state transitions, integer overflows, and underflows locally during the development lifecycle.

## 2. Team Background & Feasibility
**Lead Compiler Engineer:** Elif Nur Ayhan (@codebygunes)
As a Software Engineer specializing in deep-tech security, formal verification, and LLVM-based transformations, I have single-handedly developed the current Belit Alpha. The PoC already features fully functional native Wasm bytecode parsing (LEB128, Section decoding), SSA stack modeling, and active Z3 SMT integration via LLVM IR bridging. The core engine is fully derisked, functionally tested, and ready for advanced state modeling.

## 3. Budget & Funding Request
This $45,000 USDC funding request is designed to scale the prototype into a production-ready verifiable computation tool over a 12-week intensive R&D period. The milestones have been structured to reflect the focused bandwidth of a dedicated solo engineer.

### 3.1. Budget Breakdown & Milestones
| Milestone | Duration | Focus Area | Requested Grant Amount |
| :--- | :--- | :--- | :--- |
| **Milestone 1** | Weeks 1 - 4 | **FVM Linear Memory Modeling:** Expanding the existing `WasmLifter` to strictly map WebAssembly dynamic memory operations (`memory.grow`, `memory.size`, `i32.store`) into exact Z3 BitVector boundaries to mathematically prove the absence of out-of-bounds writes in FVM payloads. | $15,000 USDC |
| **Milestone 2** | Weeks 5 - 8 | **Advanced LLVM Deobfuscation Passes:** Optimizing the custom LLVM `DeobfuscationPass` to mathematically eliminate control flow flattening and opaque predicates in compiled Wasm binaries, mapping the cleaned SSA forms to the Z3 engine. | $15,000 USDC |
| **Milestone 3** | Weeks 9 - 12 | **FVM Ecosystem Integration & Headless CI/CD:** Delivering a fully automated verification pipeline ready for headless FVM node integration, complete with comprehensive E2E test suites, documentation, and a developer-friendly `belit` CLI. | $15,000 USDC |
| **TOTAL** | **12 Weeks** | **End-to-End FVM Verification Infrastructure** | **$45,000 USDC** |