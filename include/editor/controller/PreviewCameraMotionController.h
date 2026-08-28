#pragma once

#include "editor/controller/PreviewCameraController.h"
#include "editor/model/PreviewInteractionSettings.h"
#include "editor/model/PreviewNavigationInput.h"

class PreviewCameraMotionController
{
public:
	void advance(PreviewCameraController &camera,
	             const PreviewNavigationInput &input,
	             const PreviewInteractionSettings &settings,
	             float delta_seconds) const;
};
