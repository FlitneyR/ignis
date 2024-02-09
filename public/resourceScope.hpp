#pragma once

#include "libraries.hpp"

#include <mutex>
#include <list>

#ifndef IGNIS_RESOURCE_SCOPE_DEBUG
#define IGNIS_RESOURCE_SCOPE_DEBUG(...)
#endif

namespace ignis {

/**
 * @brief Collects cleanup commands via void addDeferredCleanupFunction(std::function<void()> func),
 *        then later executes those commands in reverse via void executeDeferredCleanupFunctions()
 */
class ResourceScope {
    std::string m_name = "";

    std::list<std::function<void()>> m_deferredCleanupCommands;

    static int s_openScopes;
    static int s_nextID;
    int m_ID = -1;

    ResourceScope(const ResourceScope& other) = delete;
    ResourceScope& operator =(const ResourceScope& other) = delete;

public:
    ResourceScope(std::string name = "");
    ~ResourceScope();

    ResourceScope(ResourceScope&& other) {
        std::swap(m_deferredCleanupCommands, other.m_deferredCleanupCommands);
        std::swap(m_ID, other.m_ID);
        std::swap(m_name, other.m_name);
    }

    ResourceScope& operator =(ResourceScope&& other) {
        std::swap(m_deferredCleanupCommands, other.m_deferredCleanupCommands);
        std::swap(m_ID, other.m_ID);
        std::swap(m_name, other.m_name);
        return *this;
    }

    void setName(std::string name) {
        IGNIS_RESOURCE_SCOPE_DEBUG("Renamed scope %s(%d) to %s\n", m_name.c_str(), m_ID, name.c_str());
        m_name = name;
    }

    void addDeferredCleanupFunction(std::function<void()> func);
    void executeDeferredCleanupFunctions();
};

}