#pragma once

#include "libraries.hpp"

#include <list>

namespace ignis {

class ResourceScope {
    std::list<std::function<void()>> m_deferredCleanupCommands;

    ResourceScope(const ResourceScope& other) = delete;
    ResourceScope(const ResourceScope&& other) = delete;
    ResourceScope& operator =(const ResourceScope& other) = delete;
    ResourceScope& operator =(const ResourceScope&& other) = delete;

public:
    ResourceScope();
    ~ResourceScope();

    void addDeferredCleanupFunction(std::function<void()> func);
    void executeDeferredCleanupFunctions();
};

}