#pragma once

#include "BackendApiClient.h"

#include <string>
#include <functional>
#include <memory>
#include <vector>

class CloudGrammarRepositoryRequest
{
public:
	virtual ~CloudGrammarRepositoryRequest() = default;
	virtual void cancel() = 0;
};

using CloudGrammarListCallback =
	std::function<void(const BackendAsyncResult<std::vector<BackendFileSummary>> &)>;
using CloudGrammarLoadCallback =
	std::function<void(const BackendAsyncResult<BackendFileRecord> &)>;

class CloudGrammarRepository
{
public:
	virtual ~CloudGrammarRepository() = default;

	virtual bool listGrammars(std::vector<BackendFileSummary> *grammars,
	                          std::string *error_message) = 0;
	virtual bool loadGrammar(const std::string &file_id,
	                         BackendFileRecord *grammar,
	                         std::string *error_message) = 0;
	virtual bool loadPublicGrammar(const std::string &file_id,
	                               BackendFileRecord *grammar,
	                               std::string *error_message) = 0;
	virtual bool saveGrammar(const std::string &file_id,
	                         const std::string &title,
	                         const std::string &source_text,
	                         BackendFileRecord *grammar,
	                         std::string *error_message) = 0;
	virtual bool publishGrammar(const std::string &file_id,
	                            const std::string &title,
	                            const std::string &source_text,
	                            BackendFileRecord *grammar,
	                            std::string *error_message) = 0;
	virtual bool unpublishGrammar(const std::string &file_id,
	                              std::string *error_message) = 0;
	virtual std::shared_ptr<CloudGrammarRepositoryRequest> requestGrammarList(
		CloudGrammarListCallback callback) = 0;
	virtual std::shared_ptr<CloudGrammarRepositoryRequest> requestGrammarLoad(
		const std::string &file_id,
		bool allow_public_fallback,
		CloudGrammarLoadCallback callback) = 0;
};
