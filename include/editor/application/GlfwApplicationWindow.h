#pragma once

#include <string>

struct GLFWwindow;

using GlfwErrorCallback = void (*)(int error_code, const char *description);

class GlfwApplicationWindow
{
public:
	GlfwApplicationWindow() = default;
	~GlfwApplicationWindow();

	GlfwApplicationWindow(const GlfwApplicationWindow &) = delete;
	GlfwApplicationWindow &operator=(const GlfwApplicationWindow &) = delete;

	bool initialize(int width,
	                int height,
	                const char *title,
	                GlfwErrorCallback error_callback,
	                std::string *error_message);

	GLFWwindow *nativeHandle() const;

private:
	bool glfw_initialized_ = false;
	GLFWwindow *window_ = nullptr;
};
