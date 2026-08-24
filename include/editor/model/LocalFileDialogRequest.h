#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

enum class LocalFileDialogPurpose {
	OpenGrammar,
	SaveGrammarAs,
	SaveBeforeDocumentReplacement
};

class LocalFileDialogFilter
{
public:
	LocalFileDialogFilter(std::string display_name, std::string extension_list)
		: display_name_(std::move(display_name)),
		  extension_list_(std::move(extension_list))
	{
	}

	const std::string &displayName() const
	{
		return display_name_;
	}

	const std::string &extensionList() const
	{
		return extension_list_;
	}

private:
	std::string display_name_;
	std::string extension_list_;
};

class LocalFileDialogRequest
{
public:
	LocalFileDialogPurpose purpose = LocalFileDialogPurpose::OpenGrammar;
	std::filesystem::path initial_directory;
	std::string suggested_filename;
	std::vector<LocalFileDialogFilter> filters;

	bool opensExistingFile() const
	{
		return purpose == LocalFileDialogPurpose::OpenGrammar;
	}
};
