#include "editor/service/NativeLocalFileDialogService.h"

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <algorithm>
#else
#define GLFW_EXPOSE_NATIVE_X11
#include <nfd_glfw3.h>
#include <nfd.hpp>
#endif

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

#if defined(_WIN32)
	(void)error_message;
#else
	if (NFD::Init() != NFD_OKAY) {
		if (error_message != nullptr) {
			const char *native_error = NFD::GetError();
			*error_message = native_error == nullptr || native_error[0] == '\0'
				? "Native file dialog initialization failed."
				: native_error;
		}
		return false;
	}
#endif

	initialized_ = true;
	return true;
}

void NativeLocalFileDialogService::shutdown()
{
	if (!initialized_) {
		return;
	}
#if !defined(_WIN32)
	NFD::Quit();
#endif
	initialized_ = false;
}

LocalFileDialogResult NativeLocalFileDialogService::show(
	const LocalFileDialogRequest &request)
{
	if (!initialized_) {
		return LocalFileDialogResult::failed("Native file dialog service is not initialized.");
	}

#if defined(_WIN32)
	std::wstring native_filters;
	for (const LocalFileDialogFilter &filter : request.filters) {
		const std::wstring extensions = std::filesystem::u8path(filter.extensionList()).wstring();
		if (extensions.empty()) {
			continue;
		}
		native_filters += std::filesystem::u8path(filter.displayName()).wstring();
		native_filters.push_back(L'\0');
		std::size_t start = 0;
		while (start < extensions.size()) {
			const std::size_t comma = extensions.find(L',', start);
			if (start != 0) {
				native_filters.push_back(L';');
			}
			native_filters += L"*." + extensions.substr(start, comma - start);
			if (comma == std::wstring::npos) {
				break;
			}
			start = comma + 1;
		}
		native_filters.push_back(L'\0');
	}

	const bool opening = request.opensExistingFile();
	std::vector<wchar_t> path_buffer(32768, L'\0');
	if (!opening) {
		const auto suggested_name = std::filesystem::u8path(request.suggested_filename).wstring();
		if (suggested_name.size() >= path_buffer.size()) {
			return LocalFileDialogResult::failed("The suggested filename is too long.");
		}
		std::copy(suggested_name.begin(), suggested_name.end(), path_buffer.begin());
	}
	const std::wstring initial_directory = request.initial_directory.wstring();
	const std::wstring extensions = request.filters.empty()
		? std::wstring()
		: std::filesystem::u8path(request.filters.front().extensionList()).wstring();
	const std::wstring extension = extensions.substr(0, extensions.find(L','));
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = parent_window_ != nullptr ? glfwGetWin32Window(parent_window_) : nullptr;
	dialog.lpstrFilter = native_filters.empty() ? nullptr : native_filters.c_str();
	dialog.nFilterIndex = 1;
	dialog.lpstrFile = path_buffer.data();
	dialog.nMaxFile = static_cast<DWORD>(path_buffer.size());
	dialog.lpstrInitialDir = initial_directory.empty() ? nullptr : initial_directory.c_str();
	dialog.lpstrDefExt = extension.empty() ? nullptr : extension.c_str();
	dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
	               (opening ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT);
	if (opening ? GetOpenFileNameW(&dialog) : GetSaveFileNameW(&dialog)) {
		return LocalFileDialogResult::selected(std::filesystem::path(path_buffer.data()));
	}
	const DWORD error = CommDlgExtendedError();
	if (error == 0) {
		return LocalFileDialogResult::cancelled();
	}
	return LocalFileDialogResult::failed("Windows file dialog failed (" + std::to_string(error) + ").");
#else
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
#endif
}
