#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "editor/service/CloudGrammarRepository.h"

class DocumentPersistenceService
{
public:
	bool loadGrammarSource(const std::filesystem::path &path,
	                       std::string *source_text,
	                       std::string *error_message) const;
	bool saveGrammarSource(const std::filesystem::path &path,
	                       const std::string &source_text,
	                       std::string *error_message) const;
	bool listCloudGrammars(CloudGrammarRepository &repository,
	                       std::vector<BackendFileSummary> *grammars,
	                       std::string *error_message) const;
	bool loadCloudGrammar(CloudGrammarRepository &repository,
	                     const std::string &file_id,
	                     bool allow_public_fallback,
	                     BackendFileRecord *grammar,
	                     std::string *error_message) const;
	bool saveCloudGrammar(CloudGrammarRepository &repository,
	                     const std::string &file_id,
	                     const std::string &title,
	                     const std::string &source_text,
	                     BackendFileRecord *grammar,
	                     std::string *error_message) const;
	bool publishCloudGrammar(CloudGrammarRepository &repository,
	                        const std::string &file_id,
	                        const std::string &title,
	                        const std::string &source_text,
	                        BackendFileRecord *grammar,
	                        std::string *error_message) const;
	bool unpublishCloudGrammar(CloudGrammarRepository &repository,
	                          const std::string &file_id,
	                          std::string *error_message) const;
	std::shared_ptr<CloudGrammarRepositoryRequest> requestCloudGrammarList(
		CloudGrammarRepository &repository,
		CloudGrammarListCallback callback) const;
	std::shared_ptr<CloudGrammarRepositoryRequest> requestCloudGrammarLoad(
		CloudGrammarRepository &repository,
		const std::string &file_id,
		bool allow_public_fallback,
		CloudGrammarLoadCallback callback,
		std::string *error_message) const;
};
