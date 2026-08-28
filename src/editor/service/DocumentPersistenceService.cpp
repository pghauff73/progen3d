#include "editor/service/DocumentPersistenceService.h"

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

bool DocumentPersistenceService::loadGrammarSource(const std::filesystem::path &path,
                                                   std::string *source_text,
                                                   std::string *error_message) const
{
	if (source_text == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Local open requires a destination document.";
		}
		return false;
	}
	if (path.empty()) {
		if (error_message != nullptr) {
			*error_message = "Enter a local grammar path to open.";
		}
		return false;
	}

	std::ifstream input(path, std::ios::binary);
	if (!input) {
		if (error_message != nullptr) {
			*error_message = "Could not open local grammar: " + path.string();
		}
		return false;
	}

	std::ostringstream contents;
	contents << input.rdbuf();
	if (!input.good() && !input.eof()) {
		if (error_message != nullptr) {
			*error_message = "Could not read local grammar: " + path.string();
		}
		return false;
	}

	*source_text = contents.str();
	return true;
}

bool DocumentPersistenceService::saveGrammarSource(const std::filesystem::path &path,
                                                   const std::string &source_text,
                                                   std::string *error_message) const
{
	if (path.empty()) {
		if (error_message != nullptr) {
			*error_message = "Enter a local grammar path to save.";
		}
		return false;
	}

	const std::filesystem::path parent_path = path.parent_path();
	if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
		if (error_message != nullptr) {
			*error_message = "Local grammar directory does not exist: " + parent_path.string();
		}
		return false;
	}

	std::filesystem::path temporary_path = path;
	temporary_path += ".tmp";
	{
		std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
		if (!output) {
			if (error_message != nullptr) {
				*error_message = "Could not create temporary grammar file: " + temporary_path.string();
			}
			return false;
		}
		output.write(source_text.data(), static_cast<std::streamsize>(source_text.size()));
		output.flush();
		if (!output) {
			if (error_message != nullptr) {
				*error_message = "Could not write local grammar: " + path.string();
			}
			output.close();
			std::error_code remove_error;
			std::filesystem::remove(temporary_path, remove_error);
			return false;
		}
	}

	std::error_code rename_error;
	std::filesystem::rename(temporary_path, path, rename_error);
	if (rename_error) {
		std::error_code remove_error;
		std::filesystem::remove(temporary_path, remove_error);
		if (error_message != nullptr) {
			*error_message = "Could not replace local grammar file: " + rename_error.message();
		}
		return false;
	}

	return true;
}

bool DocumentPersistenceService::listCloudGrammars(
	CloudGrammarRepository &repository,
	std::vector<BackendFileSummary> *grammars,
	std::string *error_message) const
{
	if (grammars == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Cloud grammar listing requires a result collection.";
		}
		return false;
	}
	return repository.listGrammars(grammars, error_message);
}

bool DocumentPersistenceService::loadCloudGrammar(
	CloudGrammarRepository &repository,
	const std::string &file_id,
	bool allow_public_fallback,
	BackendFileRecord *grammar,
	std::string *error_message) const
{
	if (file_id.empty()) {
		if (error_message != nullptr) {
			*error_message = "Cloud file id is empty.";
		}
		return false;
	}
	if (grammar == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Cloud open requires a destination document.";
		}
		return false;
	}
	std::string private_error;
	if (repository.loadGrammar(file_id, grammar, &private_error) &&
	    !grammar->id.empty() && grammar->has_content) {
		return true;
	}
	if (allow_public_fallback) {
		std::string public_error;
		if (repository.loadPublicGrammar(file_id, grammar, &public_error) &&
		    !grammar->id.empty() && grammar->has_content) {
			return true;
		}
		if (!public_error.empty()) {
			private_error = private_error.empty()
				? public_error
				: private_error + " Public fallback failed: " + public_error;
		}
	}
	if (error_message != nullptr) {
		*error_message = private_error.empty()
			? "The cloud repository did not return grammar contents."
			: private_error;
	}
	return false;
}

bool DocumentPersistenceService::saveCloudGrammar(
	CloudGrammarRepository &repository,
	const std::string &file_id,
	const std::string &title,
	const std::string &source_text,
	BackendFileRecord *grammar,
	std::string *error_message) const
{
	if (title.empty()) {
		if (error_message != nullptr) {
			*error_message = "Cloud title is empty.";
		}
		return false;
	}
	if (source_text.size() > kBackendGrammarMaxBytes) {
		if (error_message != nullptr) {
			*error_message = "Grammar exceeds the 30,000 byte backend limit.";
		}
		return false;
	}
	return repository.saveGrammar(file_id, title, source_text, grammar, error_message);
}

bool DocumentPersistenceService::publishCloudGrammar(
	CloudGrammarRepository &repository,
	const std::string &file_id,
	const std::string &title,
	const std::string &source_text,
	BackendFileRecord *grammar,
	std::string *error_message) const
{
	if (file_id.empty()) {
		if (error_message != nullptr) {
			*error_message = "Save the grammar to the cloud before publishing it.";
		}
		return false;
	}
	if (source_text.size() > kBackendGrammarMaxBytes) {
		if (error_message != nullptr) {
			*error_message = "Grammar exceeds the 30,000 byte backend limit.";
		}
		return false;
	}
	return repository.publishGrammar(file_id, title, source_text, grammar, error_message);
}

bool DocumentPersistenceService::unpublishCloudGrammar(
	CloudGrammarRepository &repository,
	const std::string &file_id,
	std::string *error_message) const
{
	if (file_id.empty()) {
		if (error_message != nullptr) {
			*error_message = "Save the grammar to the cloud before removing it from the gallery.";
		}
		return false;
	}
	return repository.unpublishGrammar(file_id, error_message);
}

std::shared_ptr<CloudGrammarRepositoryRequest>
DocumentPersistenceService::requestCloudGrammarList(
	CloudGrammarRepository &repository,
	CloudGrammarListCallback callback) const
{
	return repository.requestGrammarList(std::move(callback));
}

std::shared_ptr<CloudGrammarRepositoryRequest>
DocumentPersistenceService::requestCloudGrammarLoad(
	CloudGrammarRepository &repository,
	const std::string &file_id,
	bool allow_public_fallback,
	CloudGrammarLoadCallback callback,
	std::string *error_message) const
{
	if (file_id.empty()) {
		if (error_message != nullptr) {
			*error_message = "Cloud file id is empty.";
		}
		return {};
	}
	return repository.requestGrammarLoad(file_id,
	                                     allow_public_fallback,
	                                     std::move(callback));
}
