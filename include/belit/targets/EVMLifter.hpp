#ifndef BELIT_EVMLIFTER_HPP
#define BELIT_EVMLIFTER_HPP

#include "belit/targets/ILifter.hpp"

namespace belit {

class EVMLifter : public ILifter {
public:
    EVMLifter();
    ~EVMLifter() override = default;

    TargetArchitecture getArchitecture() const override { return TargetArchitecture::EVM; }
    std::string getArchitectureName() const override { return "Ethereum Virtual Machine (EVM)"; }

    bool parse(const std::vector<uint8_t>& rawBytecode) override;
    const std::vector<BasicBlock>& getCFG() const override;
    void reset() override;

private:
    std::vector<BasicBlock> cfg_;
};

} // namespace belit

#endif // BELIT_EVMLIFTER_HPP