#include "resourceScope.hpp"
#include "engine.hpp"

#ifndef IGNIS_RESOURCE_SCOPE_DEBUG
#define IGNIS_RESOURCE_SCOPE_DEBUG(...)
#endif

namespace ignis {

int ResourceScope::s_openScopes = 0;
int ResourceScope::s_nextID = 0;

ResourceScope::ResourceScope(
    std::string name
) : m_name(name)
{
    m_ID = s_nextID++;
    s_openScopes++;
    IGNIS_RESOURCE_SCOPE_DEBUG("Opened scope %s(%d). %d scopes open.\n", m_name.c_str(), m_ID, s_openScopes);
}

ResourceScope::~ResourceScope() {
    executeDeferredCleanupFunctions();
    s_openScopes--;
    IGNIS_RESOURCE_SCOPE_DEBUG("Closed scope %s(%d). %d scopes open.\n", m_name.c_str(), m_ID, s_openScopes);
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
