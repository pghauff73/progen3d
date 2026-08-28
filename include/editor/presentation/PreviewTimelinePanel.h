#pragma once

#include "editor/model/EditorWorkspaceSession.h"

class SceneGenerationContext;

class PreviewTimelinePanel
{
public:
	bool draw(EditorWorkspaceSession &workspace_session,
	          SceneGenerationContext *scene_context,
	          bool grammar_time_driven,
	          float current_time) const;
};
