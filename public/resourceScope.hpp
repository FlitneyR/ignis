#pragma once

#include "libraries.hpp"

#include <mutex>
#include <list>

namespace ignis {

/**
 * @brief Collects cleanup commands via void addDeferredCleanupFunction(std::function<void()> func),
 *        then later executes those commands in reverse via void executeDeferredCleanupFunctions()
 */
class ResourceScope {
    std::string m_name;

    std::list<std::function<void()>> m_deferredCleanupCommands;

public:
    ResourceScope(std::string name = "");
    ~ResourceScope();

    ResourceScope(ResourceScope&& other) = default;
    ResourceScope& operator =(ResourceScope&& other) = default;

    ResourceScope(const ResourceScope& other) = delete;
    ResourceScope& operator =(const ResourceScope& other) = delete;

    void addDeferredCleanupFunction(std::function<void()> func);
    void executeDeferredCleanupFunctions();
};

}