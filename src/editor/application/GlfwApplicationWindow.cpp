#include "editor/application/GlfwApplicationWindow.h"

#include "ProGen3dGl.h"
#include "editor/service/OpenGl46CapabilityValidator.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GlfwApplicationWindow::~GlfwApplicationWindow()
{
	if (window_ != nullptr) {
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}
	if (glfw_initialized_) {
		glfwTerminate();
		glfw_initialized_ = false;
	}
}

bool GlfwApplicationWindow::initialize(int width,
                                       int height,
                                       const char *title,
                                       GlfwErrorCallback error_callback,
                                       std::string *error_message)
{
	if (window_ != nullptr) {
		return true;
	}

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		if (error_message != nullptr) {
			*error_message = "Failed to initialize GLFW";
		}
		return false;
	}
	glfw_initialized_ = true;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (window_ == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Failed to create GLFW window";
		}
		return false;
	}

	glfwMakeContextCurrent(window_);
	glfwSwapInterval(1);
	if (!progen3d_initialize_gl_loader((GLADloadfunc)glfwGetProcAddress)) {
		if (error_message != nullptr) {
			*error_message = "Failed to initialize OpenGL loader";
		}
		return false;
	}
	const OpenGl46CapabilityValidator capability_validator;
	if (!capability_validator.validate(error_message)) {
		return false;
	}

	return true;
}

GLFWwindow *GlfwApplicationWindow::nativeHandle() const
{
	return window_;
}
