#ifndef BELIT_WASMLIFTER_HPP
#define BELIT_WASMLIFTER_HPP

#include "belit/targets/ILifter.hpp"
#include <string>
#include <vector>

namespace belit {

class WasmLifter : public ILifter {
public:
    WasmLifter();
    ~WasmLifter() override = default;

    TargetArchitecture getArchitecture() const override { return TargetArchitecture::WASM; }
    std::string getArchitectureName() const override { return "WebAssembly (WASM)"; }

    bool parse(const std::vector<uint8_t>& rawBytecode) override;
    const std::vector<BasicBlock>& getCFG() const override;
    void reset() override;

    // --- NEW: Access to FVM function names extracted from Wasm Section 2 ---
    const std::vector<std::string>& getImportedFunctions() const { return importedFunctions_; }

private:
    std::vector<BasicBlock> cfg_;
    std::vector<std::string> importedFunctions_; // FVM host call names
};

} // namespace belit

#endif // BELIT_WASMLIFTER_HPP