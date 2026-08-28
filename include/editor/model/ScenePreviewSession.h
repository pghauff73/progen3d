#pragma once

#include "editor/controller/PreviewCameraController.h"
#include "editor/model/EditorSelection.h"
#include "editor/model/GeneratedSceneSnapshot.h"
#include "editor/model/PreviewProjectionConfiguration.h"
#include "editor/model/PreviewInteractionSettings.h"
#include "editor/model/PreviewTimelineState.h"
#include "editor/model/PreviewStatistics.h"
#include "editor/model/RenderConfiguration.h"
#include "editor/model/SceneOverlayConfiguration.h"
#include "editor/relationship/SceneSourceAssociationIndex.h"
#include "lighting/model/LightingSceneState.h"

#include <memory>

class ScenePreviewSession
{
public:
	std::unique_ptr<GeneratedSceneSnapshot> last_valid_scene_snapshot;
	PreviewTimelineState timeline;
	PreviewCameraController camera;
	PreviewProjectionConfiguration projection;
	PreviewInteractionSettings interaction_settings;
	RenderConfiguration render_configuration;
	EditorSelection selection;
	SceneOverlayConfiguration overlays;
	PreviewStatistics statistics;
	SceneSourceAssociationIndex source_associations;
	LightingSceneState lighting;
	bool fit_camera_pending = true;
};
