#pragma once

#include "AppPaths.h"
#include "editor/application/GlfwApplicationWindow.h"
#include "editor/application/ImGuiRuntimeContext.h"
#include "editor/service/StlPartCatalogRepository.h"

#include <functional>
#include <string>

class EditorRuntimeEnvironment
{
public:
	EditorRuntimeEnvironment() = default;
	~EditorRuntimeEnvironment();

	EditorRuntimeEnvironment(const EditorRuntimeEnvironment &) = delete;
	EditorRuntimeEnvironment &operator=(const EditorRuntimeEnvironment &) = delete;

	bool initialize(int window_width,
	                int window_height,
	                const char *window_title,
	                GlfwErrorCallback error_callback,
	                const std::function<void()> &apply_interface_style,
	                std::string *error_message);
	void shutdown();

	GLFWwindow *window() const;
	ApplicationPathLocator &pathLocator();
	PartCatalogRepository &partCatalogRepository();

private:
	ProgenyApplicationPathLocator application_path_locator_;
	StlPartCatalogRepository part_catalog_repository_;
	GlfwApplicationWindow application_window_;
	ImGuiRuntimeContext imgui_runtime_context_;
	bool renderer_initialized_ = false;
};
