#pragma once

#include "editor/application/ApplicationLaunchOptions.h"
#include "editor/model/EditorWorkspaceSession.h"

class Progen3dEditorApplication
{
public:
	explicit Progen3dEditorApplication(ApplicationLaunchOptions launch_options);

	int run();

private:
	ApplicationLaunchOptions launch_options_;
	EditorWorkspaceSession workspace_session_;
};
