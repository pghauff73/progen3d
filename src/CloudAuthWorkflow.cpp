#include "CloudAuthWorkflow.h"

#include "editor/service/BackendCloudGrammarRepository.h"
#include "editor/service/DocumentPersistenceService.h"
#include "editor/service/FirebaseAuthenticationService.h"

#include <algorithm>
#include <utility>

void set_auth_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->authentication.status_message = message;
	app_state->authentication.status_is_error = is_error;
}

FirebaseAuthConfig current_auth_config_from_inputs(const EditorWorkspaceSession *app_state)
{
	FirebaseAuthConfig config;
	if (app_state == nullptr) {
		return config;
	}
	config = app_state->authentication.config;
	config.api_key = text_buffer_value(app_state->authentication.api_key);
	config.project_id = text_buffer_value(app_state->authentication.project_id);
	config.google_client_id = text_buffer_value(app_state->authentication.google_client_id);
	config.google_client_secret = text_buffer_value(app_state->authentication.google_client_secret);
	config.backend_base_url = text_buffer_value(app_state->authentication.backend_base_url);
	if (config.auth_domain.empty() && !config.project_id.empty()) {
		config.auth_domain = config.project_id + ".firebaseapp.com";
	}
	if (config.storage_bucket.empty() && !config.project_id.empty()) {
		config.storage_bucket = config.project_id + ".firebasestorage.app";
	}
	return config;
}

void load_firebase_config_into_ui(EditorWorkspaceSession *app_state, const FirebaseAuthConfig &config)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->authentication.config = config;
	set_text_buffer(&app_state->authentication.api_key, config.api_key);
	set_text_buffer(&app_state->authentication.project_id, config.project_id);
	set_text_buffer(&app_state->authentication.google_client_id, config.google_client_id);
	set_text_buffer(&app_state->authentication.google_client_secret, config.google_client_secret);
	set_text_buffer(&app_state->authentication.backend_base_url, config.backend_base_url);
}

void apply_authenticated_session(EditorWorkspaceSession *app_state,
                                 const FirebaseAuthSession &session,
                                 const std::string &message)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->authentication.session = session;
	app_state->authentication.session.authenticated = true;
	set_text_buffer(&app_state->authentication.email, session.email);
	set_text_buffer(&app_state->authentication.password, "");
	if (!message.empty()) {
		set_auth_feedback(app_state, message, false);
	}
}

void clear_cloud_document_identity(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->storage_identity.clearCloudIdentity();
}

void clear_ai_assistant_state(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->ai_assistant = AiAssistantSession{};
}

void set_cloud_document_identity(EditorWorkspaceSession *app_state,
                                 const std::string &file_id,
                                 const std::string &title,
                                 bool published)
{
	if (app_state == nullptr) {
		return;
	}
	const std::string normalized_title = trim_copy(title);
	app_state->storage_identity.adoptCloudIdentity(
		trim_copy(file_id), normalized_title, published);
	if (!normalized_title.empty()) {
		app_state->document.title = normalized_title;
	}
}

bool synchronize_backend_login(EditorWorkspaceSession *app_state,
                               FirebaseAuthSession *session,
                               std::string *error)
{
	if (app_state == nullptr) {
		return false;
	}

	app_state->authentication.backend_user = BackendUserProfile{};
	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	if (trim_copy(config.backend_base_url).empty()) {
		clear_texture_library_state(app_state);
		clear_ai_assistant_state(app_state);
		invalidate_texture_materials(app_state);
		return true;
	}

	BackendUserProfile backend_user;
	if (!backend_api_login(config, session, &backend_user, error)) {
		clear_texture_library_state(app_state);
		clear_ai_assistant_state(app_state);
		invalidate_texture_materials(app_state);
		return false;
	}
	app_state->authentication.backend_user = std::move(backend_user);
	refresh_texture_library(app_state, true);
	refresh_ai_threads(app_state, true);
	return true;
}

void load_detected_firebase_config(EditorWorkspaceSession *app_state, bool show_feedback)
{
	if (app_state == nullptr) {
		return;
	}
	FirebaseAuthConfig detected_config;
	std::string error;
	if (load_firebase_auth_config(&detected_config, &error)) {
		load_firebase_config_into_ui(app_state, detected_config);
		if (show_feedback) {
			set_auth_feedback(app_state,
			                  "Loaded Firebase config from " + detected_config.loaded_from + ".",
			                  false);
		}
		return;
	}
	if (show_feedback) {
		set_auth_feedback(app_state,
		                  "No Firebase config was found. Enter your API key and project ID, or add firebase_auth_config.json / google-services.json.",
		                  true);
	}
}

void initialize_auth_state(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	load_detected_firebase_config(app_state, false);
	if (app_state->authentication.config.valid()) {
		set_auth_feedback(app_state,
		                  "Loaded Firebase config from " + app_state->authentication.config.loaded_from + ".",
		                  false);
	} else {
		set_auth_feedback(app_state,
		                  "Enter your Firebase API key and project ID, or add firebase_auth_config.json / google-services.json.",
		                  false);
	}

	FirebaseAuthSession saved_session;
	std::string session_error;
	if (!load_firebase_auth_session(&saved_session, &session_error)) {
		return;
	}

	if (!saved_session.email.empty()) {
		set_text_buffer(&app_state->authentication.email, saved_session.email);
	}
	if (!app_state->authentication.config.valid()) {
		set_auth_feedback(app_state,
		                  "A saved Firebase session exists, but no Firebase config could be loaded.",
		                  true);
		return;
	}

	std::string refresh_error;
	FirebaseAuthSession refreshed_session = saved_session;
	FirebaseAuthenticationService authentication_service;
	if (authentication_service.refreshSession(app_state->authentication.config,
	                                          &refreshed_session,
	                                          &refresh_error)) {
		std::string backend_error;
		const bool backend_synced =
			synchronize_backend_login(app_state, &refreshed_session, &backend_error);
		apply_authenticated_session(app_state,
		                            refreshed_session,
		                            refreshed_session.email.empty()
		                                ? "Restored Firebase session."
		                                : "Restored Firebase session for " + refreshed_session.email + ".");
		if (!backend_synced) {
			const std::string message_prefix =
				refreshed_session.email.empty()
					? "Restored Firebase session."
					: "Restored Firebase session for " + refreshed_session.email + ".";
			set_auth_feedback(app_state,
			                  message_prefix + " Cloud backend sync is unavailable: " + backend_error,
			                  true);
		}
		std::string save_error;
		if (!save_firebase_auth_session(refreshed_session, &save_error)) {
			debugout("Firebase session refresh succeeded but saving the session failed: " + save_error);
		}
		return;
	}

	clear_firebase_auth_session_file();
	set_auth_feedback(app_state, refresh_error.empty() ? session_error : refresh_error, true);
}

void sign_out_firebase(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	clear_firebase_auth_session_file();
	app_state->authentication.session = FirebaseAuthSession{};
	app_state->authentication.backend_user = BackendUserProfile{};
	clear_texture_library_state(app_state);
	clear_ai_assistant_state(app_state);
	invalidate_texture_materials(app_state);
	cancel_cloud_dialog_async_requests(app_state);
	CloudDialogState &cloud = app_state->cloud_dialog;
	cloud.action = CloudDialogAction::None;
	cloud.selected_file_id.clear();
	cloud.name_input.clear();
	cloud.entries.clear();
	cloud.status_message.clear();
	cloud.status_is_error = false;
	cloud.request_open = false;
	cloud.request_focus = false;
	clear_cloud_document_identity(app_state);
	backend_api_clear_cache();
	set_text_buffer(&app_state->authentication.password, "");
	set_auth_feedback(app_state, "Signed out of Firebase.", false);
}

std::string normalize_cloud_title(std::string title)
{
	return trim_copy(title);
}

std::string default_cloud_title(const EditorWorkspaceSession *app_state)
{
	if (app_state != nullptr && !app_state->storage_identity.cloud_title.empty()) {
		return app_state->storage_identity.cloud_title;
	}
	return "Untitled grammar";
}

void set_cloud_dialog_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->cloud_dialog.status_message = message;
	app_state->cloud_dialog.status_is_error = is_error;
}

void persist_current_firebase_session(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || !app_state->authentication.session.authenticated) {
		return;
	}
	std::string save_error;
	save_firebase_auth_session(app_state->authentication.session, &save_error);
}

void cancel_cloud_dialog_async_requests(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	CloudDialogState &state = app_state->cloud_dialog;
	if (state.list_request != nullptr) {
		state.list_request->cancel();
	}
	if (state.open_request != nullptr) {
		state.open_request->cancel();
	}
	state.list_request.reset();
	state.open_request.reset();
	state.loading_entries = false;
	state.opening_file = false;

	if (state.async_results != nullptr) {
		std::lock_guard<std::mutex> lock(state.async_results->mutex);
		state.async_results->list_result_ready = false;
		state.async_results->list_result_success = false;
		state.async_results->open_result_ready = false;
		state.async_results->open_result_success = false;
		state.async_results->pending_entries.clear();
		state.async_results->pending_file_record = BackendFileRecord{};
		state.async_results->pending_error.clear();
	}
}

void request_cloud_dialog_refresh(EditorWorkspaceSession *app_state, const std::string &message)
{
	if (app_state == nullptr) {
		return;
	}

	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	if (!config.valid()) {
		set_cloud_dialog_feedback(app_state,
		                          "Firebase config is incomplete. Check the API key and project ID.",
		                          true);
		return;
	}
	if (trim_copy(config.backend_base_url).empty()) {
		set_cloud_dialog_feedback(app_state,
		                          "Backend URL is not configured. Set backendBaseUrl in the Firebase config.",
		                          true);
		return;
	}

	CloudDialogState &state = app_state->cloud_dialog;
	if (state.list_request != nullptr) {
		state.list_request->cancel();
		state.list_request.reset();
	}
	state.loading_entries = true;
	set_cloud_dialog_feedback(app_state, message, false);

	const std::shared_ptr<CloudDialogAsyncResults> async_results = state.async_results;
	if (async_results == nullptr) {
		state.loading_entries = false;
		set_cloud_dialog_feedback(app_state, "Cloud result storage is unavailable.", true);
		return;
	}
	{
		std::lock_guard<std::mutex> lock(async_results->mutex);
		async_results->list_result_ready = false;
		async_results->list_result_success = false;
		async_results->pending_entries.clear();
		async_results->pending_error.clear();
	}

	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	state.list_request = persistence_service.requestCloudGrammarList(
		repository,
		[async_results](const BackendAsyncResult<std::vector<BackendFileSummary>> &result) {
			std::lock_guard<std::mutex> lock(async_results->mutex);
			async_results->list_result_ready = true;
			async_results->list_result_success = result.success;
			async_results->pending_entries =
				result.success ? result.value : std::vector<BackendFileSummary>{};
			async_results->pending_error = result.success
				? std::string{}
				: (result.error.empty() ? "Unable to list cloud files." : result.error);
		});
}

void request_cloud_document_open(EditorWorkspaceSession *app_state,
                                 const std::string &file_id,
                                 bool allow_public_fallback)
{
	if (app_state == nullptr) {
		return;
	}

	CloudDialogState &state = app_state->cloud_dialog;
	const std::string normalized_file_id = trim_copy(file_id);
	if (normalized_file_id.empty()) {
		set_cloud_dialog_feedback(app_state, "Cloud file id is empty.", true);
		return;
	}

	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	if (!config.valid()) {
		set_cloud_dialog_feedback(app_state,
		                          "Firebase config is incomplete. Check the API key and project ID.",
		                          true);
		return;
	}
	if (trim_copy(config.backend_base_url).empty()) {
		set_cloud_dialog_feedback(app_state,
		                          "Backend URL is not configured. Set backendBaseUrl in the Firebase config.",
		                          true);
		return;
	}

	if (state.open_request != nullptr) {
		state.open_request->cancel();
		state.open_request.reset();
	}
	state.opening_file = true;
	set_cloud_dialog_feedback(app_state, "Opening cloud file...", false);

	const std::shared_ptr<CloudDialogAsyncResults> async_results = state.async_results;
	if (async_results == nullptr) {
		state.opening_file = false;
		set_cloud_dialog_feedback(app_state, "Cloud result storage is unavailable.", true);
		return;
	}
	{
		std::lock_guard<std::mutex> lock(async_results->mutex);
		async_results->open_result_ready = false;
		async_results->open_result_success = false;
		async_results->pending_file_record = BackendFileRecord{};
		async_results->pending_error.clear();
	}

	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	std::string request_error;
	state.open_request = persistence_service.requestCloudGrammarLoad(
		repository,
		normalized_file_id,
		allow_public_fallback,
		[async_results](const BackendAsyncResult<BackendFileRecord> &result) {
			std::lock_guard<std::mutex> lock(async_results->mutex);
			async_results->open_result_ready = true;
			async_results->open_result_success = result.success;
			async_results->pending_file_record = result.success
				? result.value
				: BackendFileRecord{};
			async_results->pending_error = result.success
				? std::string{}
				: (result.error.empty() ? "Unable to open cloud file." : result.error);
		},
		&request_error);
	if (state.open_request == nullptr) {
		state.opening_file = false;
		set_cloud_dialog_feedback(app_state,
		                          request_error.empty() ? "Unable to start the cloud open request."
		                                                : request_error,
		                          true);
	}
}

bool apply_cloud_dialog_async_results(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return false;
	}

	CloudDialogState &state = app_state->cloud_dialog;
	bool list_result_ready = false;
	bool list_result_success = false;
	std::vector<BackendFileSummary> list_entries;
	std::string list_error;
	bool open_result_ready = false;
	bool open_result_success = false;
	BackendFileRecord opened_file;
	std::string open_error;
	const std::shared_ptr<CloudDialogAsyncResults> async_results = state.async_results;
	if (async_results == nullptr) {
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(async_results->mutex);
		if (async_results->list_result_ready) {
			list_result_ready = true;
			list_result_success = async_results->list_result_success;
			list_entries = std::move(async_results->pending_entries);
			if (!list_result_success) {
				list_error = async_results->pending_error;
			}
			async_results->list_result_ready = false;
			async_results->pending_entries.clear();
		}
		if (async_results->open_result_ready) {
			open_result_ready = true;
			open_result_success = async_results->open_result_success;
			opened_file = std::move(async_results->pending_file_record);
			if (!open_result_success) {
				open_error = async_results->pending_error;
			}
			async_results->open_result_ready = false;
			async_results->pending_file_record = BackendFileRecord{};
		}
	}

	if (list_result_ready) {
		state.loading_entries = false;
		if (list_result_success) {
			state.entries = std::move(list_entries);
			set_cloud_dialog_feedback(app_state, "", false);
		} else {
			set_cloud_dialog_feedback(app_state,
			                          list_error.empty() ? "Unable to list cloud files." : list_error,
			                          true);
		}
	}

	if (!open_result_ready) {
		return false;
	}

	state.opening_file = false;
	if (!open_result_success || opened_file.id.empty() || !opened_file.has_content) {
		const std::string message =
			open_error.empty()
				? "Failed to open the cloud file: the backend did not return file contents."
				: open_error;
		errorout(message);
		set_cloud_dialog_feedback(app_state, message, true);
		return false;
	}

	persist_current_firebase_session(app_state);
	set_editor_document(app_state, opened_file.content, false);
	set_cloud_document_identity(app_state,
	                            opened_file.id,
	                            opened_file.title,
	                            opened_file.is_published);
	state.selected_file_id = opened_file.id;
	state.name_input = opened_file.title;
	debugout("Loaded cloud file: " + opened_file.title + " (" + opened_file.id + ")");
	request_scene_regeneration(app_state->document.source_text);
	set_cloud_dialog_feedback(app_state, "", false);
	return true;
}

bool refresh_cloud_dialog_entries(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return false;
	}

	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	if (!config.valid()) {
		set_cloud_dialog_feedback(app_state,
		                          "Firebase config is incomplete. Check the API key and project ID.",
		                          true);
		return false;
	}
	if (trim_copy(config.backend_base_url).empty()) {
		set_cloud_dialog_feedback(app_state,
		                          "Backend URL is not configured. Set backendBaseUrl in the Firebase config.",
		                          true);
		return false;
	}

	std::vector<BackendFileSummary> grammars;
	std::string error;
	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	if (!persistence_service.listCloudGrammars(repository, &grammars, &error)) {
		set_cloud_dialog_feedback(app_state,
		                          error.empty() ? "Unable to list cloud files." : error,
		                          true);
		return false;
	}

	persist_current_firebase_session(app_state);
	app_state->cloud_dialog.entries = std::move(grammars);
	set_cloud_dialog_feedback(app_state, "", false);
	return true;
}

bool save_document_to_cloud(EditorWorkspaceSession *app_state,
                            const std::string &requested_title,
                            bool create_new_file)
{
	if (app_state == nullptr) {
		return false;
	}

	const std::string cloud_title = normalize_cloud_title(requested_title);
	if (cloud_title.empty()) {
		errorout("Cloud title is empty");
		return false;
	}
	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	const std::string file_id = create_new_file ? "" : trim_copy(app_state->storage_identity.cloud_file_id);
	BackendFileRecord file_record;
	std::string error;
	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	if (!persistence_service.saveCloudGrammar(repository,
	                                         file_id,
	                                         cloud_title,
	                                         app_state->document.source_text,
	                                         &file_record,
	                                         &error)) {
		errorout(error.empty() ? "Failed to save grammar to the backend" : error);
		set_cloud_dialog_feedback(app_state, error, true);
		return false;
	}

	persist_current_firebase_session(app_state);
	set_cloud_document_identity(app_state,
	                            file_record.id,
	                            file_record.title.empty() ? cloud_title : file_record.title,
	                            false);
	app_state->document.markPersisted();
	app_state->cloud_dialog.selected_file_id = app_state->storage_identity.cloud_file_id;
	app_state->cloud_dialog.name_input = app_state->storage_identity.cloud_title;
	debugout("Saved cloud file: " + app_state->storage_identity.cloud_title + " (" + app_state->storage_identity.cloud_file_id + ")");
	if (!refresh_cloud_dialog_entries(app_state)) {
		debugout("Cloud save succeeded but refreshing the cloud list failed.");
	}
	set_cloud_dialog_feedback(app_state, "Saved to cloud.", false);
	return true;
}

bool open_document_from_cloud(EditorWorkspaceSession *app_state,
                              const std::string &requested_file_id,
                              bool allow_public_fallback)
{
	if (app_state == nullptr) {
		return false;
	}

	const std::string file_id = trim_copy(requested_file_id);
	if (file_id.empty()) {
		errorout("Cloud file id is empty");
		return false;
	}

	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	BackendFileRecord file_record;
	std::string error;
	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	if (!persistence_service.loadCloudGrammar(repository,
	                                         file_id,
	                                         allow_public_fallback,
	                                         &file_record,
	                                         &error)) {
		const std::string message =
			error.empty()
				? "Failed to open the cloud file: the backend did not return file contents."
				: error;
		errorout(message);
		set_cloud_dialog_feedback(app_state, message, true);
		return false;
	}

	persist_current_firebase_session(app_state);
	set_editor_document(app_state, file_record.content, false);
	set_cloud_document_identity(app_state,
	                            file_record.id,
	                            file_record.title,
	                            file_record.is_published);
	app_state->cloud_dialog.selected_file_id = file_record.id;
	app_state->cloud_dialog.name_input = file_record.title;
	debugout("Loaded cloud file: " + file_record.title + " (" + file_record.id + ")");
	request_scene_regeneration(app_state->document.source_text);
	set_cloud_dialog_feedback(app_state, "", false);
	return true;
}

bool publish_document_to_cloud(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return false;
	}
	if (trim_copy(app_state->storage_identity.cloud_file_id).empty()) {
		errorout("Save the grammar to the cloud before publishing it");
		return false;
	}
	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	BackendFileRecord file_record;
	std::string error;
	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	if (!persistence_service.publishCloudGrammar(repository,
	                                            app_state->storage_identity.cloud_file_id,
	                                            default_cloud_title(app_state),
	                                            app_state->document.source_text,
	                                            &file_record,
	                                            &error)) {
		errorout(error.empty() ? "Failed to publish the cloud file" : error);
		return false;
	}

	persist_current_firebase_session(app_state);
	app_state->document.markPersisted();
	set_cloud_document_identity(app_state,
	                            file_record.id.empty() ? app_state->storage_identity.cloud_file_id : file_record.id,
	                            file_record.title.empty() ? default_cloud_title(app_state) : file_record.title,
	                            true);
	debugout("Published cloud file: " + app_state->storage_identity.cloud_title + " (" + app_state->storage_identity.cloud_file_id + ")");
	if (!refresh_cloud_dialog_entries(app_state)) {
		debugout("Cloud publish succeeded but refreshing the cloud list failed.");
	}
	set_cloud_dialog_feedback(app_state, "Published to gallery.", false);
	return true;
}

bool depublish_document_from_cloud(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return false;
	}
	if (trim_copy(app_state->storage_identity.cloud_file_id).empty()) {
		errorout("Save the grammar to the cloud before depublishing it");
		return false;
	}

	const FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
	std::string error;
	BackendCloudGrammarRepository repository(config, app_state->authentication.session);
	DocumentPersistenceService persistence_service;
	if (!persistence_service.unpublishCloudGrammar(repository,
	                                              app_state->storage_identity.cloud_file_id,
	                                              &error)) {
		errorout(error.empty() ? "Failed to depublish the cloud file" : error);
		return false;
	}

	persist_current_firebase_session(app_state);
	app_state->storage_identity.markUnpublished();
	debugout("Depublished cloud file: " + app_state->storage_identity.cloud_title + " (" + app_state->storage_identity.cloud_file_id + ")");
	if (!refresh_cloud_dialog_entries(app_state)) {
		debugout("Cloud depublish succeeded but refreshing the cloud list failed.");
	}
	set_cloud_dialog_feedback(app_state, "Removed from gallery.", false);
	return true;
}

void create_new_document(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	set_editor_document(app_state, default_new_document_text(), false);
	debugout("Created new grammar document");
	request_scene_regeneration(app_state->document.source_text);
}
