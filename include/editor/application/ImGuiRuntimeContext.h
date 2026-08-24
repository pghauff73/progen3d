#pragma once

#include <string>

struct GLFWwindow;

class ImGuiRuntimeContext
{
public:
	ImGuiRuntimeContext() = default;
	~ImGuiRuntimeContext();

	ImGuiRuntimeContext(const ImGuiRuntimeContext &) = delete;
	ImGuiRuntimeContext &operator=(const ImGuiRuntimeContext &) = delete;

	bool create(std::string *error_message);
	bool initializeBackends(GLFWwindow *window,
	                        const char *glsl_version,
	                        std::string *error_message);
	void shutdown();

private:
	bool context_created_ = false;
	bool glfw_backend_initialized_ = false;
	bool opengl_backend_initialized_ = false;
};
