#pragma once

#include "editor/service/CloudGrammarRepository.h"

class BackendCloudGrammarRepository final : public CloudGrammarRepository
{
public:
	BackendCloudGrammarRepository(const FirebaseAuthConfig &configuration,
	                              FirebaseAuthSession &session);

	bool listGrammars(std::vector<BackendFileSummary> *grammars,
	                  std::string *error_message) override;
	bool loadGrammar(const std::string &file_id,
	                 BackendFileRecord *grammar,
	                 std::string *error_message) override;
	bool loadPublicGrammar(const std::string &file_id,
	                       BackendFileRecord *grammar,
	                       std::string *error_message) override;
	bool saveGrammar(const std::string &file_id,
	                 const std::string &title,
	                 const std::string &source_text,
	                 BackendFileRecord *grammar,
	                 std::string *error_message) override;
	bool publishGrammar(const std::string &file_id,
	                    const std::string &title,
	                    const std::string &source_text,
	                    BackendFileRecord *grammar,
	                    std::string *error_message) override;
	bool unpublishGrammar(const std::string &file_id,
	                      std::string *error_message) override;
	std::shared_ptr<CloudGrammarRepositoryRequest> requestGrammarList(
		CloudGrammarListCallback callback) override;
	std::shared_ptr<CloudGrammarRepositoryRequest> requestGrammarLoad(
		const std::string &file_id,
		bool allow_public_fallback,
		CloudGrammarLoadCallback callback) override;

private:
	FirebaseAuthConfig configuration_;
	FirebaseAuthSession &session_;
};
