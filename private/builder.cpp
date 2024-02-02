#include "builder.hpp"
#include "engine.hpp"

namespace ignis {

vk::Device BuilderBase::getDevice() {
    return IEngine::get().getDevice();
}

VmaAllocator BuilderBase::getAllocator() {
    return IEngine::get().getAllocator();
}

}
