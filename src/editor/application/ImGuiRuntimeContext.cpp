#include "editor/application/ImGuiRuntimeContext.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImGuiRuntimeContext::~ImGuiRuntimeContext()
{
	shutdown();
}

bool ImGuiRuntimeContext::create(std::string *error_message)
{
	if (context_created_) {
		return true;
	}

	IMGUI_CHECKVERSION();
	if (ImGui::CreateContext() == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Failed to create Dear ImGui context";
		}
		return false;
	}
	context_created_ = true;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	return true;
}

bool ImGuiRuntimeContext::initializeBackends(GLFWwindow *window,
                                             const char *glsl_version,
                                             std::string *error_message)
{
	if (!context_created_) {
		if (error_message != nullptr) {
			*error_message = "Dear ImGui context must be created before its backends";
		}
		return false;
	}
	if (window == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Dear ImGui GLFW backend requires a window";
		}
		return false;
	}

	if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
		if (error_message != nullptr) {
			*error_message = "Failed to initialize Dear ImGui GLFW backend";
		}
		return false;
	}
	glfw_backend_initialized_ = true;

	if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
		if (error_message != nullptr) {
			*error_message = "Failed to initialize Dear ImGui OpenGL backend";
		}
		return false;
	}
	opengl_backend_initialized_ = true;
	return true;
}

void ImGuiRuntimeContext::shutdown()
{
	if (opengl_backend_initialized_) {
		ImGui_ImplOpenGL3_Shutdown();
		opengl_backend_initialized_ = false;
	}
	if (glfw_backend_initialized_) {
		ImGui_ImplGlfw_Shutdown();
		glfw_backend_initialized_ = false;
	}
	if (context_created_) {
		ImGui::DestroyContext();
		context_created_ = false;
	}
}
