#pragma once

#include "editor/model/LocalFileDialogRequest.h"

#include <filesystem>
#include <optional>
#include <string>
#include <utility>

class LocalDocumentWorkflowState
{
public:
	std::string status_message;
	bool status_is_error = false;
	std::filesystem::path last_successful_directory;
	std::optional<LocalFileDialogRequest> pending_dialog_request;

	void requestDialog(LocalFileDialogRequest request)
	{
		pending_dialog_request = std::move(request);
	}

	std::optional<LocalFileDialogRequest> takePendingDialogRequest()
	{
		std::optional<LocalFileDialogRequest> request = std::move(pending_dialog_request);
		pending_dialog_request.reset();
		return request;
	}

	void reset()
	{
		status_message.clear();
		status_is_error = false;
		pending_dialog_request.reset();
	}
};
