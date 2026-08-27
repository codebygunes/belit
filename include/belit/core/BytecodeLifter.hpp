#pragma once

#include "belit/core/Common.hpp"
#include <span>

namespace belit {

// Interface for target-agnostic bytecode lifting
class IBytecodeLifter {
public:
    virtual ~IBytecodeLifter() = default;
    
    virtual bool parse(std::span<const uint8_t> rawBytecode) = 0;
    virtual const std::vector<BasicBlock>& getCFG() const = 0;
    virtual std::string getTargetName() const = 0;
};

} // namespace belit