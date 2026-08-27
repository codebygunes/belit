# Contributing to Belit

First off, thank you for taking the time to contribute! Belit is an open-source formal verification and deobfuscation engine committed to secure software systems.

## Code of Conduct
By participating, you are expected to uphold our open and welcoming community standards.

## How Can I Contribute?

### Reporting Bugs
If you find a bug in the source code or a discrepancy in the Z3 mathematical verification models, please open an issue with:
* A clear description of the problem.
* Minimal reproduction bytecode or IR.
* Expected vs. actual verification output.

### Pull Requests
1. **Fork** the repository and create your branch from `main`.
2. Ensure your code follows the C++20 standard and passes all formatting checks.
3. Add or update unit/E2E tests in the `tests/` directory for any new logic or bug fix.
4. Run the local test suite to ensure **100% test pass rate**:
   ```bash
   cmake -B build
   cmake --build build -j 4
   cd build
   ctest --output-on-failure