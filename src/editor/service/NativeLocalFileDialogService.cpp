#include "editor/service/NativeLocalFileDialogService.h"

#define GLFW_EXPOSE_NATIVE_X11
#include <nfd_glfw3.h>
#include <nfd.hpp>

#include <vector>

NativeLocalFileDialogService::NativeLocalFileDialogService(GLFWwindow *parent_window)
	: parent_window_(parent_window)
{
}

NativeLocalFileDialogService::~NativeLocalFileDialogService()
{
	shutdown();
}

bool NativeLocalFileDialogService::initialize(std::string *error_message)
{
	if (initialized_) {
		return true;
	}

	if (NFD::Init() != NFD_OKAY) {
		if (error_message != nullptr) {
			const char *native_error = NFD::GetError();
			*error_message = native_error == nullptr || native_error[0] == '\0'
				? "Native file dialog initialization failed."
				: native_error;
		}
		return false;
	}

	initialized_ = true;
	return true;
}

void NativeLocalFileDialogService::shutdown()
{
	if (!initialized_) {
		return;
	}
	NFD::Quit();
	initialized_ = false;
}

LocalFileDialogResult NativeLocalFileDialogService::show(
	const LocalFileDialogRequest &request)
{
	if (!initialized_) {
		return LocalFileDialogResult::failed("Native file dialog service is not initialized.");
	}

	std::vector<nfdnfilteritem_t> native_filters;
	native_filters.reserve(request.filters.size());
	for (const LocalFileDialogFilter &filter : request.filters) {
		native_filters.push_back({filter.displayName().c_str(), filter.extensionList().c_str()});
	}

	nfdwindowhandle_t parent_window{};
	if (parent_window_ != nullptr) {
		NFD_GetNativeWindowFromGLFWWindow(parent_window_, &parent_window);
	}

	const std::string initial_directory = request.initial_directory.string();
	const nfdnchar_t *default_path = initial_directory.empty() ? nullptr : initial_directory.c_str();
	nfdnchar_t *selected_path = nullptr;
	nfdresult_t result = NFD_CANCEL;
	if (request.opensExistingFile()) {
		result = NFD::OpenDialog(selected_path,
		                         native_filters.empty() ? nullptr : native_filters.data(),
		                         static_cast<nfdfiltersize_t>(native_filters.size()),
		                         default_path,
		                         parent_window);
	} else {
		const nfdnchar_t *default_name = request.suggested_filename.empty()
			? nullptr
			: request.suggested_filename.c_str();
		result = NFD::SaveDialog(selected_path,
		                         native_filters.empty() ? nullptr : native_filters.data(),
		                         static_cast<nfdfiltersize_t>(native_filters.size()),
		                         default_path,
		                         default_name,
		                         parent_window);
	}

	if (result == NFD_CANCEL) {
		return LocalFileDialogResult::cancelled();
	}
	if (result != NFD_OKAY || selected_path == nullptr) {
		const char *native_error = NFD::GetError();
		return LocalFileDialogResult::failed(
			native_error == nullptr || native_error[0] == '\0'
				? "Native file dialog failed."
				: native_error);
	}

	const std::filesystem::path chosen_path(selected_path);
	NFD::FreePath(selected_path);
	return LocalFileDialogResult::selected(chosen_path);
}
