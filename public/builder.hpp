#pragma once

#include "resourceScope.hpp"

namespace ignis {

class BuilderBase {
protected:
    ResourceScope& r_scope;

public:
    BuilderBase(ResourceScope& scope) : r_scope(scope) {}

    vk::Device   getDevice();
    VmaAllocator getAllocator();
};

template<typename Type>
class IBuilder : public BuilderBase {
public:
    IBuilder(ResourceScope& scope) : BuilderBase(scope) {}

    virtual Type build() = 0;
};

}
