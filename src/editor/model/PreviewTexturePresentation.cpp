#include "editor/model/PreviewTexturePresentation.h"

#include <algorithm>

PreviewTextureCoordinate PreviewTexturePresentation::upperLeftTextureCoordinate() const
{
	return PreviewTextureCoordinate{0.0f, 1.0f};
}

PreviewTextureCoordinate PreviewTexturePresentation::lowerRightTextureCoordinate() const
{
	return PreviewTextureCoordinate{1.0f, 0.0f};
}

float PreviewTexturePresentation::normalizedDeviceXFromViewportFraction(
	float fraction_from_left) const
{
	const float bounded_fraction = std::clamp(fraction_from_left, 0.0f, 1.0f);
	return bounded_fraction * 2.0f - 1.0f;
}

float PreviewTexturePresentation::normalizedDeviceYFromViewportFraction(
	float fraction_from_top) const
{
	const float bounded_fraction = std::clamp(fraction_from_top, 0.0f, 1.0f);
	return 1.0f - bounded_fraction * 2.0f;
}
