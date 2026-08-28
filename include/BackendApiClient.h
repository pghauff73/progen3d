#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "FirebaseAuth.h"

constexpr std::size_t kBackendGrammarMaxBytes = 30000u;
constexpr std::size_t kBackendTextureSlotCount = 20u;

class BackendDataRecord {
public:
	virtual ~BackendDataRecord() = default;
};

class BackendLoadedRecord : public BackendDataRecord {
public:
	bool loaded = false;
};

class BackendIdentifiedRecord : public BackendDataRecord {
public:
	std::string id;
};

class BackendTimestampedRecord : public BackendDataRecord {
public:
	std::string updated_at;
};

class BackendIdentifiedTimestampedRecord : public BackendIdentifiedRecord, public BackendTimestampedRecord {
};

class BackendCreditSummary : public BackendLoadedRecord {
public:
	int balance = 0;
	int available = 0;
	int granted_lifetime = 0;
	int spent_lifetime = 0;
	int reserved = 0;
	std::string plan;
	std::string updated_at;
};

class BackendAiPreferences : public BackendDataRecord {
public:
	std::string ai_model;
	std::string ai_image_model;
};

class BackendUserProfile : public BackendIdentifiedRecord {
public:
	bool authenticated = false;
	std::string username;
	std::string email;
	std::string role;
	BackendCreditSummary credits;
	BackendAiPreferences preferences;
};

class BackendFileSummary : public BackendIdentifiedTimestampedRecord {
public:
	std::string title;
	bool is_published = false;
};

class BackendFileRecord : public BackendFileSummary {
public:
	std::string content;
	bool has_content = false;
};

class BackendTextureSlot : public BackendTimestampedRecord {
public:
	std::string slot;
	std::string display_name;
	bool active = false;
	float alpha = 1.0f;
	int width = 512;
	int height = 512;
	std::string source;
	std::string prompt;
	std::string image_url;
};

class BackendAiThreadSummary : public BackendIdentifiedTimestampedRecord {
public:
	std::string title;
	std::string mode;
	std::string file_id;
	std::string file_title;
	std::string created_at;
	std::string last_message_at;
	std::string last_message_preview;
	int message_count = 0;
};

class BackendAiUsageSummary : public BackendLoadedRecord, public BackendIdentifiedRecord {
public:
	std::string status;
	int estimated_credits = 0;
	int final_credits = 0;
	int prompt_tokens = 0;
	int completion_tokens = 0;
	int total_tokens = 0;
};

class BackendAiResult : public BackendLoadedRecord {
public:
	std::string raw_json;
	std::string title;
	std::string grammar;
	std::string summary;
	std::string answer;
	std::string repair_summary;
	std::string lesson;
	std::string diagnosis;
	std::string practice_prompt;
	std::vector<std::string> motifs;
	std::vector<std::string> next_steps;
	std::vector<std::string> changes;
	std::vector<std::string> observations;
	std::vector<std::string> suggested_edits;
	std::vector<std::string> actions;
	std::vector<std::string> warnings;
};

class BackendAiMessage : public BackendIdentifiedRecord {
public:
	std::string thread_id;
	std::string owner_uid;
	std::string role;
	std::string mode;
	std::string content;
	std::string created_at;
	std::string payload_json;
	BackendAiResult result;
};

class BackendRequestRecord : public BackendDataRecord {
public:
	virtual bool isValid() const
	{
		return true;
	}
};

class BackendAiGenerateRequest : public BackendRequestRecord {
public:
	std::string mode = "active_helper_chat";
	std::string prompt;
	std::string question;
	std::string grammar;
	std::string selection;
	std::string parser_error;
	std::string thread_id;
	std::string file_id;
	std::string title;
	std::vector<std::string> history;

	bool isValid() const override
	{
		return !mode.empty();
	}
};

class BackendResponseRecord : public BackendDataRecord {
public:
	virtual ~BackendResponseRecord() = default;
};

class BackendOperationStatus : public BackendResponseRecord {
public:
	bool ready = false;
	bool success = false;
	std::string error;

	void markSucceeded()
	{
		ready = true;
		success = true;
		error.clear();
	}

	void markFailed(const std::string &message)
	{
		ready = true;
		success = false;
		error = message;
	}
};

template <typename T>
class BackendValueResponse : public BackendOperationStatus {
public:
	T value{};
};

class BackendOperationToken : public BackendDataRecord {
public:
	virtual ~BackendOperationToken() = default;
};

class BackendAsyncOperationToken : public BackendOperationToken {
public:
	BackendAsyncOperationToken() = default;

	explicit BackendAsyncOperationToken(std::shared_ptr<std::atomic<bool>> cancellation_state)
		: cancelled(std::move(cancellation_state))
	{
	}

	std::shared_ptr<std::atomic<bool>> cancelled;
};

class BackendAiGenerateResponse : public BackendResponseRecord {
public:
	std::string mode;
	std::string model;
	BackendAiThreadSummary thread;
	BackendCreditSummary credits;
	BackendAiUsageSummary usage;
	std::vector<BackendAiMessage> messages;
	BackendAiResult result;
};

template <typename T>
using BackendAsyncResult = BackendValueResponse<T>;

using BackendAsyncToken = BackendAsyncOperationToken;

using BackendListFilesCallback =
	std::function<void(const BackendAsyncResult<std::vector<BackendFileSummary>> &)>;

using BackendGetFileCallback =
	std::function<void(const BackendAsyncResult<BackendFileRecord> &)>;

bool backend_api_login(const FirebaseAuthConfig &config,
                       FirebaseAuthSession *session,
                       BackendUserProfile *profile = nullptr,
                       std::string *error = nullptr);
bool backend_api_list_files(const FirebaseAuthConfig &config,
                            FirebaseAuthSession *session,
                            std::vector<BackendFileSummary> *files,
                            std::string *error = nullptr);
bool backend_api_get_file(const FirebaseAuthConfig &config,
                          FirebaseAuthSession *session,
                          const std::string &file_id,
                          BackendFileRecord *file,
                          std::string *error = nullptr);
bool backend_api_get_public_file(const FirebaseAuthConfig &config,
                                 FirebaseAuthSession *session,
                                 const std::string &file_id,
                                 BackendFileRecord *file,
                                 std::string *error = nullptr);
bool backend_api_save_file(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           const std::string &file_id,
                           const std::string &title,
                           const std::string &content,
                           BackendFileRecord *file,
                           std::string *error = nullptr);
bool backend_api_publish_file(const FirebaseAuthConfig &config,
                              FirebaseAuthSession *session,
                              const std::string &file_id,
                              const std::string &title,
                              const std::string &content,
                              BackendFileRecord *file,
                              std::string *error = nullptr);
bool backend_api_unpublish_file(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &file_id,
                                std::string *error = nullptr);
bool backend_api_list_textures(const FirebaseAuthConfig &config,
                               FirebaseAuthSession *session,
                               std::vector<BackendTextureSlot> *textures,
                               std::string *error = nullptr);
bool backend_api_fetch_texture_image(const FirebaseAuthConfig &config,
                                     FirebaseAuthSession *session,
                                     const std::string &slot,
                                     std::string *png_bytes,
                                     std::string *error = nullptr);
bool backend_api_update_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                const std::string &display_name,
                                float alpha,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error = nullptr);
bool backend_api_delete_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error = nullptr);
bool backend_api_upload_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                const std::string &display_name,
                                float alpha,
                                const std::string &local_path,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error = nullptr);
bool backend_api_generate_texture(const FirebaseAuthConfig &config,
                                  FirebaseAuthSession *session,
                                  const std::string &slot,
                                  const std::string &display_name,
                                  float alpha,
                                  const std::string &prompt,
                                  std::vector<BackendTextureSlot> *textures,
                                  BackendCreditSummary *credits = nullptr,
                                  std::string *error = nullptr);
bool backend_api_list_ai_threads(const FirebaseAuthConfig &config,
                                 FirebaseAuthSession *session,
                                 std::vector<BackendAiThreadSummary> *threads,
                                 std::string *error = nullptr);
bool backend_api_get_ai_thread(const FirebaseAuthConfig &config,
                               FirebaseAuthSession *session,
                               const std::string &thread_id,
                               BackendAiThreadSummary *thread,
                               std::vector<BackendAiMessage> *messages,
                               std::string *error = nullptr);
bool backend_api_generate_ai(const FirebaseAuthConfig &config,
                             FirebaseAuthSession *session,
                             const BackendAiGenerateRequest &request,
                             BackendAiGenerateResponse *response,
                             std::string *error = nullptr);

BackendAsyncToken backend_api_list_files_async(const FirebaseAuthConfig &config,
                                               FirebaseAuthSession session,
                                               BackendListFilesCallback callback);
BackendAsyncToken backend_api_get_file_async(const FirebaseAuthConfig &config,
                                             FirebaseAuthSession session,
                                             std::string file_id,
                                             BackendGetFileCallback callback);
void backend_api_cancel_async(const BackendAsyncToken &token);
bool backend_api_try_get_cached_file_list(std::vector<BackendFileSummary> *files);
bool backend_api_try_get_cached_file(const std::string &file_id, BackendFileRecord *file);
void backend_api_clear_cache();
