#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "FirebaseAuth.h"

constexpr std::size_t kFirebaseGrammarMaxBytes = 30000u;

class FirebaseStorageRecord {
public:
	virtual ~FirebaseStorageRecord() = default;

	std::string object_path;
	std::size_t size_bytes = 0;
	std::string updated;
};

class FirebaseNamedStorageRecord : public FirebaseStorageRecord {
public:
	std::string name;
};

class FirebasePrivateGrammarRecord : public FirebaseNamedStorageRecord {
public:
	bool published = false;
};

using FirebaseStoredGrammar = FirebasePrivateGrammarRecord;

class FirebaseGrammarStorageService {
public:
	virtual ~FirebaseGrammarStorageService() = default;

	virtual bool listPrivateGrammars(const FirebaseAuthConfig &config,
	                                 FirebaseAuthSession *session,
	                                 std::vector<FirebasePrivateGrammarRecord> *grammars,
	                                 std::string *error = nullptr) = 0;
	virtual bool loadPrivateGrammar(const FirebaseAuthConfig &config,
	                                FirebaseAuthSession *session,
	                                const std::string &grammar_name,
	                                std::string *contents,
	                                bool *published = nullptr,
	                                std::string *error = nullptr) = 0;
	virtual bool savePrivateGrammar(const FirebaseAuthConfig &config,
	                                FirebaseAuthSession *session,
	                                const std::string &grammar_name,
	                                const std::string &contents,
	                                std::string *error = nullptr) = 0;
	virtual bool publishGrammar(const FirebaseAuthConfig &config,
	                            FirebaseAuthSession *session,
	                            const std::string &grammar_name,
	                            const std::string &contents,
	                            const std::string &author_email,
	                            std::string *error = nullptr) = 0;
	virtual bool depublishGrammar(const FirebaseAuthConfig &config,
	                              FirebaseAuthSession *session,
	                              const std::string &grammar_name,
	                              std::string *error = nullptr) = 0;
};

class FirebaseCloudGrammarStorageService : public FirebaseGrammarStorageService {
public:
	bool listPrivateGrammars(const FirebaseAuthConfig &config,
	                         FirebaseAuthSession *session,
	                         std::vector<FirebasePrivateGrammarRecord> *grammars,
	                         std::string *error = nullptr) override;
	bool loadPrivateGrammar(const FirebaseAuthConfig &config,
	                        FirebaseAuthSession *session,
	                        const std::string &grammar_name,
	                        std::string *contents,
	                        bool *published = nullptr,
	                        std::string *error = nullptr) override;
	bool savePrivateGrammar(const FirebaseAuthConfig &config,
	                        FirebaseAuthSession *session,
	                        const std::string &grammar_name,
	                        const std::string &contents,
	                        std::string *error = nullptr) override;
	bool publishGrammar(const FirebaseAuthConfig &config,
	                    FirebaseAuthSession *session,
	                    const std::string &grammar_name,
	                    const std::string &contents,
	                    const std::string &author_email,
	                    std::string *error = nullptr) override;
	bool depublishGrammar(const FirebaseAuthConfig &config,
	                      FirebaseAuthSession *session,
	                      const std::string &grammar_name,
	                      std::string *error = nullptr) override;
};

FirebaseGrammarStorageService &firebase_grammar_storage_service();

bool firebase_storage_list_private_grammars(const FirebaseAuthConfig &config,
                                            FirebaseAuthSession *session,
                                            std::vector<FirebaseStoredGrammar> *grammars,
                                            std::string *error = nullptr);
bool firebase_storage_load_private_grammar(const FirebaseAuthConfig &config,
                                           FirebaseAuthSession *session,
                                           const std::string &grammar_name,
                                           std::string *contents,
                                           bool *published = nullptr,
                                           std::string *error = nullptr);
bool firebase_storage_save_private_grammar(const FirebaseAuthConfig &config,
                                           FirebaseAuthSession *session,
                                           const std::string &grammar_name,
                                           const std::string &contents,
                                           std::string *error = nullptr);
bool firebase_storage_publish_grammar(const FirebaseAuthConfig &config,
                                      FirebaseAuthSession *session,
                                      const std::string &grammar_name,
                                      const std::string &contents,
                                      const std::string &author_email,
                                      std::string *error = nullptr);
bool firebase_storage_depublish_grammar(const FirebaseAuthConfig &config,
                                        FirebaseAuthSession *session,
                                        const std::string &grammar_name,
                                        std::string *error = nullptr);
