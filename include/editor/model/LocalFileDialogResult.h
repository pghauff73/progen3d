#pragma once

#include <filesystem>
#include <string>
#include <utility>

enum class LocalFileDialogOutcome {
	Selected,
	Cancelled,
	Failed
};

class LocalFileDialogResult
{
public:
	static LocalFileDialogResult selected(std::filesystem::path path)
	{
		LocalFileDialogResult result;
		result.outcome_ = LocalFileDialogOutcome::Selected;
		result.selected_path_ = std::move(path);
		return result;
	}

	static LocalFileDialogResult cancelled()
	{
		LocalFileDialogResult result;
		result.outcome_ = LocalFileDialogOutcome::Cancelled;
		return result;
	}

	static LocalFileDialogResult failed(std::string error_message)
	{
		LocalFileDialogResult result;
		result.outcome_ = LocalFileDialogOutcome::Failed;
		result.error_message_ = std::move(error_message);
		return result;
	}

	LocalFileDialogOutcome outcome() const
	{
		return outcome_;
	}

	const std::filesystem::path &selectedPath() const
	{
		return selected_path_;
	}

	const std::string &errorMessage() const
	{
		return error_message_;
	}

private:
	LocalFileDialogOutcome outcome_ = LocalFileDialogOutcome::Cancelled;
	std::filesystem::path selected_path_;
	std::string error_message_;
};
