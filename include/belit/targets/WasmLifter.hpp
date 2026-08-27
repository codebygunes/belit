#ifndef BELIT_WASMLIFTER_HPP
#define BELIT_WASMLIFTER_HPP

#include "belit/targets/ILifter.hpp"

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

private:
    std::vector<BasicBlock> cfg_;
};

} // namespace belit

#endif // BELIT_WASMLIFTER_HPP