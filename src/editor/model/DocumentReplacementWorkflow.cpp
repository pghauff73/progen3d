#include "editor/model/DocumentReplacementWorkflow.h"

bool DocumentReplacementWorkflow::request(DocumentReplacementAction action,
                                           bool document_is_dirty)
{
	pending_action_ = action;
	confirmation_requested_ = document_is_dirty;
	return !document_is_dirty;
}

bool DocumentReplacementWorkflow::confirmationIsPending() const
{
	return pending_action_ != DocumentReplacementAction::None;
}

bool DocumentReplacementWorkflow::consumeConfirmationRequest()
{
	const bool was_requested = confirmation_requested_;
	confirmation_requested_ = false;
	return was_requested;
}

DocumentReplacementAction DocumentReplacementWorkflow::authorizeReplacement()
{
	const DocumentReplacementAction authorized_action = pending_action_;
	pending_action_ = DocumentReplacementAction::None;
	confirmation_requested_ = false;
	return authorized_action;
}

void DocumentReplacementWorkflow::cancelReplacement()
{
	pending_action_ = DocumentReplacementAction::None;
	confirmation_requested_ = false;
}

DocumentReplacementAction DocumentReplacementWorkflow::pendingAction() const
{
	return pending_action_;
}
