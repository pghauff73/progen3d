#pragma once

struct PreviewTextureCoordinate
{
	float horizontal = 0.0f;
	float vertical = 0.0f;
};

class PreviewTexturePresentation
{
public:
	PreviewTextureCoordinate upperLeftTextureCoordinate() const;
	PreviewTextureCoordinate lowerRightTextureCoordinate() const;

	float normalizedDeviceXFromViewportFraction(float fraction_from_left) const;
	float normalizedDeviceYFromViewportFraction(float fraction_from_top) const;
};
