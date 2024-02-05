#include "resourceScope.hpp"
#include "engine.hpp"

namespace ignis {

ResourceScope::ResourceScope(
    std::string name
) : m_name(name)
{}

ResourceScope::~ResourceScope() {
    executeDeferredCleanupFunctions();
}

void ResourceScope::addDeferredCleanupFunction(std::function<void()> func) {
    m_deferredCleanupCommands.push_back(func);
}

void ResourceScope::executeDeferredCleanupFunctions() {
    if (!m_name.empty())
        IGNIS_LOG("Resource Scope", Info, "Cleaning up: " << m_name);

    while (!m_deferredCleanupCommands.empty()) {
        m_deferredCleanupCommands.back()();
        m_deferredCleanupCommands.pop_back();
    }
}

}
