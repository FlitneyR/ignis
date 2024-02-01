#pragma once

#include "libraries.hpp"

namespace ignis {

template<typename T>
T getValue(vk::ResultValue<T> rv, const char* message) {
    vk::resultCheck(rv.result, message);
    return rv.value;
}

template<typename T>
T getValue(vkb::Result<T> rv, const char* message) {
    if (!rv) throw std::runtime_error(message);
    return *rv;
}

}
