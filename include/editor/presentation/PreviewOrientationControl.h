#pragma once

#include "editor/controller/PreviewCameraController.h"

#include <optional>

class PreviewOrientationControl
{
public:
	std::optional<PreviewOrientation> orientationForNormal(float x, float y, float z) const;
};
