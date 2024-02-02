#include "allocated.hpp"
#include "engine.hpp"

namespace ignis {

VmaAllocator BaseAllocated::getAllocator() {
    return IEngine::get().getAllocator();
}

}
