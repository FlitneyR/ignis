#include "resourceScope.hpp"

namespace ignis {

ResourceScope::ResourceScope() {}

ResourceScope::~ResourceScope() {
    executeDeferredCleanupFunctions();
}

void ResourceScope::addDeferredCleanupFunction(std::function<void()> func) {
    m_deferredCleanupCommands.push_back(func);
}

void ResourceScope::executeDeferredCleanupFunctions() {
    while (!m_deferredCleanupCommands.empty()) {
        m_deferredCleanupCommands.back()();
        m_deferredCleanupCommands.pop_back();
    }
}

}
