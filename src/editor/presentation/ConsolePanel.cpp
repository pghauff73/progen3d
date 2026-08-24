#include "editor/presentation/ConsolePanel.h"

#include <imgui.h>

#include <string>
#include <vector>

void ConsolePanel::draw(ApplicationLog &application_log, ImFont *monospace_font) const
{
	const std::vector<std::string> messages = application_log.snapshot();
	ImGui::TextUnformatted("Console");
	ImGui::SameLine();
	ImGui::TextDisabled("%zu lines", messages.size());
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		application_log.clear();
	}
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.965f, 0.973f, 0.984f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.847f, 0.878f, 0.910f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
	ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), true);
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(messages.size()));
	if (monospace_font != nullptr) {
		ImGui::PushFont(monospace_font);
	}
	while (clipper.Step()) {
		for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line) {
			const std::string &message = messages[static_cast<std::size_t>(line)];
			if (message.rfind("[DEBUG]", 0) == 0) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.561f, 0.596f, 0.671f, 1.0f));
				ImGui::TextWrapped("%s", message.c_str());
				ImGui::PopStyleColor();
			} else {
				ImGui::TextWrapped("%s", message.c_str());
			}
		}
	}
	if (monospace_font != nullptr) {
		ImGui::PopFont();
	}
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}
