#pragma once

#include "libraries.hpp"

#include <list>

namespace ignis {

/**
 * @brief Collects cleanup commands via void addDeferredCleanuFunction(std::function<void()> func),
 *        then later executes those commands in reverse via void executeDeferredCleanupFunctions()
 */
class ResourceScope {
    std::list<std::function<void()>> m_deferredCleanupCommands;

    ResourceScope(ResourceScope&& other) = delete;
    ResourceScope(const ResourceScope& other) = delete;
    ResourceScope& operator =(ResourceScope&& other) = delete;
    ResourceScope& operator =(const ResourceScope& other) = delete;

public:
    ResourceScope();
    ~ResourceScope();

    void addDeferredCleanupFunction(std::function<void()> func);
    void executeDeferredCleanupFunctions();
};

}