#pragma once

#include "editor/service/LocalFileDialogService.h"

#include <string>

struct GLFWwindow;

class NativeLocalFileDialogService final : public LocalFileDialogService
{
public:
	explicit NativeLocalFileDialogService(GLFWwindow *parent_window);
	~NativeLocalFileDialogService() override;

	NativeLocalFileDialogService(const NativeLocalFileDialogService &) = delete;
	NativeLocalFileDialogService &operator=(const NativeLocalFileDialogService &) = delete;

	bool initialize(std::string *error_message);
	void shutdown();
	LocalFileDialogResult show(const LocalFileDialogRequest &request) override;

private:
	GLFWwindow *parent_window_ = nullptr;
	bool initialized_ = false;
};
