#include "log.hpp"
#include "string"

namespace ignis {

std::string Log::Entry::asString() {
    return category + std::string(s_typeNames[static_cast<uint32_t>(type)]) + message;
}

void Log::addEntry(Log::Entry entry) {
    m_entries.push_back(entry);
}

void Log::draw() {
    ImGui::Begin("Log");

    if (ImGui::Button("Clear")) m_entries.clear();

    ImGui::SameLine();
    m_filter.Draw();

    if (ImGui::BeginChild("scroll box", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    if (ImGui::BeginTable("Entries", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_WidthStretch
                                    | ImGuiTableColumnFlags_NoHeaderWidth;

        ImGui::TableSetupColumn("Category", flags, 0.f);
        ImGui::TableSetupColumn("Type", flags, 0.f);
        ImGui::TableSetupColumn("Message", flags, 3.f);
        ImGui::TableHeadersRow();

        for (auto& entry : m_entries) {
            std::string str = entry.asString();

            if (m_filter.PassFilter(&str.front(), &str.back())) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.category.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", s_typeNames[static_cast<uint32_t>(entry.type)]);
                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.message.c_str());
            }
        }

        ImGui::EndTable();
    }

    if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

}
