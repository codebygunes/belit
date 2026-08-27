#ifndef BELIT_ILIFTER_HPP
#define BELIT_ILIFTER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include "belit/core/Common.hpp"

namespace belit {

enum class TargetArchitecture {
    EVM,
    WASM,
    EBPF,
    RISCV
};

class ILifter {
public:
    virtual ~ILifter() = default;

    virtual TargetArchitecture getArchitecture() const = 0;
    virtual std::string getArchitectureName() const = 0;
    
    virtual bool parse(const std::vector<uint8_t>& rawBytecode) = 0;
    virtual const std::vector<BasicBlock>& getCFG() const = 0;
    virtual void reset() = 0;
};

} // namespace belit

#endif // BELIT_ILIFTER_HPP