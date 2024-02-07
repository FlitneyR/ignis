#pragma once

#include "libraries.hpp"
#include <mutex>

namespace ignis {

class Log {
public:
    enum class Type : uint8_t {
        Verbose = 0,
        Info,
        Warning,
        Error,
    };

    struct Entry {
        std::string category;
        Type type;
        std::string message;

        std::string asString();
    };

    void addEntry(Entry entry);
    void draw();

private:
    static constexpr const char* s_typeNames[5] {
        "Verbose", "Info", "Warning", "Error"
    };

    std::vector<Entry> m_entries;
    ImGuiTextFilter    m_filter;
    std::mutex         m_mutex;

};

}
