#include "editor/presentation/PreviewOrientationControl.h"

std::optional<PreviewOrientation> PreviewOrientationControl::orientationForNormal(
	float x,
	float y,
	float z) const
{
	if (x > 0.5f) {
		return PreviewOrientation::PositiveX;
	}
	if (x < -0.5f) {
		return PreviewOrientation::NegativeX;
	}
	if (y > 0.5f) {
		return PreviewOrientation::PositiveY;
	}
	if (y < -0.5f) {
		return PreviewOrientation::NegativeY;
	}
	if (z > 0.5f) {
		return PreviewOrientation::PositiveZ;
	}
	if (z < -0.5f) {
		return PreviewOrientation::NegativeZ;
	}
	return std::nullopt;
}
