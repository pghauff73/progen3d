#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "BackendApiClient.h"
#include "editor/model/AiAssistantSession.h"
#include "editor/model/AuthenticatedUserSession.h"
#include "editor/model/DocumentDiagnosticCollection.h"
#include "editor/model/DocumentReplacementWorkflow.h"
#include "editor/model/DocumentStorageIdentity.h"
#include "editor/model/GrammarSourceDocument.h"
#include "editor/model/LocalDocumentWorkflowState.h"
#include "editor/model/ScenePreviewSession.h"
#include "editor/model/WorkspaceLayoutState.h"
#include "editor/service/CloudGrammarRepository.h"

enum class CloudDialogAction {
	None,
	Open,
	SaveAs
};

class CloudDialogAsyncResults
{
public:
	std::mutex mutex;
	bool list_result_ready = false;
	bool list_result_success = false;
	bool open_result_ready = false;
	bool open_result_success = false;
	std::vector<BackendFileSummary> pending_entries;
	BackendFileRecord pending_file_record;
	std::string pending_error;
};

struct CloudDialogState {
	CloudDialogAction action = CloudDialogAction::None;
	std::string selected_file_id;
	std::string name_input;
	std::vector<BackendFileSummary> entries;
	std::string status_message;
	bool status_is_error = false;
	bool request_open = false;
	bool request_focus = false;
	bool loading_entries = false;
	bool opening_file = false;
	std::shared_ptr<CloudDialogAsyncResults> async_results =
		std::make_shared<CloudDialogAsyncResults>();
	std::shared_ptr<CloudGrammarRepositoryRequest> list_request;
	std::shared_ptr<CloudGrammarRepositoryRequest> open_request;
};

struct TextureLibraryState {
	std::vector<BackendTextureSlot> entries;
	std::string selected_slot = "usertexture1";
	std::string display_name_input = "usertexture1";
	std::string prompt_input;
	std::string upload_path_input;
	std::string status_message;
	bool status_is_error = false;
	float alpha_value = 1.0f;
	unsigned int preview_texture = 0;
	int preview_width = 0;
	int preview_height = 0;
	std::string preview_slot;
	std::string preview_updated_at;
};

class EditorWorkspaceSession
{
public:
	EditorWorkspaceSession() = default;

	EditorWorkspaceSession(const EditorWorkspaceSession &) = delete;
	EditorWorkspaceSession &operator=(const EditorWorkspaceSession &) = delete;
	EditorWorkspaceSession(EditorWorkspaceSession &&) = delete;
	EditorWorkspaceSession &operator=(EditorWorkspaceSession &&) = delete;

	GrammarSourceDocument document;
	DocumentStorageIdentity storage_identity;
	LocalDocumentWorkflowState local_document_workflow;
	DocumentReplacementWorkflow document_replacement_workflow;
	DocumentDiagnosticCollection document_diagnostics;
	ScenePreviewSession preview;
	WorkspaceLayoutState layout;
	AuthenticatedUserSession authentication;
	AiAssistantSession ai_assistant;
	CloudDialogState cloud_dialog;
	TextureLibraryState texture_library;
};
