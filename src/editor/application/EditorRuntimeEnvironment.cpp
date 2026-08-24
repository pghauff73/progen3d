#include "editor/application/EditorRuntimeEnvironment.h"

#include "imgui_render.h"

EditorRuntimeEnvironment::~EditorRuntimeEnvironment()
{
	shutdown();
}

bool EditorRuntimeEnvironment::initialize(int window_width,
                                          int window_height,
                                          const char *window_title,
                                          GlfwErrorCallback error_callback,
                                          const std::function<void()> &apply_interface_style,
                                          std::string *error_message)
{
	if (!application_window_.initialize(window_width,
	                                    window_height,
	                                    window_title,
	                                    error_callback,
	                                    error_message)) {
		return false;
	}

	if (!initialize_renderer()) {
		if (error_message != nullptr) {
			*error_message = "Failed to initialize renderer";
		}
		return false;
	}
	renderer_initialized_ = true;

	if (!imgui_runtime_context_.create(error_message)) {
		return false;
	}
	if (apply_interface_style) {
		apply_interface_style();
	}
	if (!imgui_runtime_context_.initializeBackends(application_window_.nativeHandle(),
	                                               "#version 460 core",
	                                               error_message)) {
		return false;
	}

	return true;
}

void EditorRuntimeEnvironment::shutdown()
{
	imgui_runtime_context_.shutdown();
	if (renderer_initialized_) {
		shutdown_renderer();
		renderer_initialized_ = false;
	}
}

GLFWwindow *EditorRuntimeEnvironment::window() const
{
	return application_window_.nativeHandle();
}

ApplicationPathLocator &EditorRuntimeEnvironment::pathLocator()
{
	return application_path_locator_;
}

PartCatalogRepository &EditorRuntimeEnvironment::partCatalogRepository()
{
	return part_catalog_repository_;
}
