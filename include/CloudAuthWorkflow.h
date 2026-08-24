#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "editor/model/EditorWorkspaceSession.h"

template <std::size_t N>
void set_text_buffer(std::array<char, N> *buffer, const std::string &value)
{
	if (buffer == nullptr || N == 0) {
		return;
	}

	buffer->fill('\0');
	const std::size_t copy_length = std::min<std::size_t>(value.size(), N - 1);
	for (std::size_t index = 0; index < copy_length; ++index) {
		(*buffer)[index] = value[index];
	}
}

template <std::size_t N>
std::string text_buffer_value(const std::array<char, N> &buffer)
{
	return std::string(buffer.data());
}

std::string trim_copy(const std::string &input);
void debugout(const std::string &message);
void errorout(std::string error_str);
void clear_texture_library_state(EditorWorkspaceSession *app_state);
void refresh_texture_library(EditorWorkspaceSession *app_state, bool preserve_status_message = false);
void refresh_ai_threads(EditorWorkspaceSession *app_state, bool preserve_status_message = false);
void invalidate_texture_materials(EditorWorkspaceSession *app_state);
void set_editor_document(EditorWorkspaceSession *app_state, const std::string &text, bool dirty);
std::string default_new_document_text();
void request_scene_regeneration(const std::string &source_text);

void set_auth_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error);
FirebaseAuthConfig current_auth_config_from_inputs(const EditorWorkspaceSession *app_state);
void load_firebase_config_into_ui(EditorWorkspaceSession *app_state, const FirebaseAuthConfig &config);
void apply_authenticated_session(EditorWorkspaceSession *app_state,
                                 const FirebaseAuthSession &session,
                                 const std::string &message);
void clear_cloud_document_identity(EditorWorkspaceSession *app_state);
void clear_ai_assistant_state(EditorWorkspaceSession *app_state);
void set_cloud_document_identity(EditorWorkspaceSession *app_state,
                                 const std::string &file_id,
                                 const std::string &title,
                                 bool published);
bool synchronize_backend_login(EditorWorkspaceSession *app_state,
                               FirebaseAuthSession *session,
                               std::string *error);
void load_detected_firebase_config(EditorWorkspaceSession *app_state, bool show_feedback);
void initialize_auth_state(EditorWorkspaceSession *app_state);
void sign_out_firebase(EditorWorkspaceSession *app_state);

std::string normalize_cloud_title(std::string title);
std::string default_cloud_title(const EditorWorkspaceSession *app_state);
void set_cloud_dialog_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error);
void persist_current_firebase_session(EditorWorkspaceSession *app_state);
void cancel_cloud_dialog_async_requests(EditorWorkspaceSession *app_state);
void request_cloud_dialog_refresh(EditorWorkspaceSession *app_state, const std::string &message);
void request_cloud_document_open(EditorWorkspaceSession *app_state,
                                 const std::string &file_id,
                                 bool allow_public_fallback);
bool apply_cloud_dialog_async_results(EditorWorkspaceSession *app_state);
bool refresh_cloud_dialog_entries(EditorWorkspaceSession *app_state);
bool save_document_to_cloud(EditorWorkspaceSession *app_state,
                            const std::string &requested_title,
                            bool create_new_file);
bool open_document_from_cloud(EditorWorkspaceSession *app_state,
                              const std::string &requested_file_id,
                              bool allow_public_fallback);
bool publish_document_to_cloud(EditorWorkspaceSession *app_state);
bool depublish_document_from_cloud(EditorWorkspaceSession *app_state);
void create_new_document(EditorWorkspaceSession *app_state);
