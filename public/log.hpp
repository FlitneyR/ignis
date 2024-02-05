#pragma once

#include "libraries.hpp"

#define IGNIS_LOG(category, type, msg) { \
    std::stringstream ss; \
    ss << msg; \
    ignis::IEngine::get().getLog().addEntry({ category, ignis::Log::Type::type, ss.str() }); \
    }

namespace ignis {

class Log {
public:
    enum class Type : uint8_t {
        Info = 0,
        Warning,
        Debug,
        Verbose,
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
    static constexpr std::array<const char*, 4> s_typeNames {
        "Info", "Warning", "Verbose"
    };

    std::vector<Entry> m_entries;
    ImGuiTextFilter    m_filter;

};

}
