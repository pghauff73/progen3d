#pragma once

#include "editor/model/EditorWorkspaceSession.h"

class ScenePreviewPanel
{
public:
	void draw(EditorWorkspaceSession &workspace_session, float delta_seconds) const;
};
