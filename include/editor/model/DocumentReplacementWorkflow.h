#pragma once

enum class DocumentReplacementAction {
	None,
	CreateNewDocument,
	OpenLocalDocument,
	OpenCloudDocument,
	ExitApplication
};

class DocumentReplacementWorkflow
{
public:
	bool request(DocumentReplacementAction action, bool document_is_dirty);

	bool confirmationIsPending() const;
	bool consumeConfirmationRequest();
	DocumentReplacementAction authorizeReplacement();
	void cancelReplacement();

	DocumentReplacementAction pendingAction() const;

private:
	DocumentReplacementAction pending_action_ = DocumentReplacementAction::None;
	bool confirmation_requested_ = false;
};
