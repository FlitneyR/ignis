#pragma once

#include "libraries.hpp"
#include "resourceScope.hpp"

namespace ignis {

template<typename Type>
class IBuilder {
protected:
    vk::Device m_device;
    ResourceScope& m_scope;

public:
    IBuilder(
        vk::Device device,
        ResourceScope& scope
    ) : m_device(device),
        m_scope(scope)
    {}

    virtual Type build() = 0;
};

}
