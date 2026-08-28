#include "lighting/service/PhotometricColorService.h"

#include <algorithm>
#include <cmath>

namespace {

float srgb_to_linear(float channel)
{
	const float clamped = std::clamp(channel, 0.0f, 1.0f);
	return clamped <= 0.04045f
		? clamped / 12.92f
		: std::pow((clamped + 0.055f) / 1.055f, 2.4f);
}

}

glm::vec3 PhotometricColorService::kelvinToLinearRgb(float kelvin) const
{
	const float temperature = std::clamp(kelvin, 1000.0f, 40000.0f) / 100.0f;
	float red = 255.0f;
	float green = 255.0f;
	float blue = 255.0f;
	if (temperature <= 66.0f) {
		green = 99.4708025861f * std::log(temperature) - 161.1195681661f;
		blue = temperature <= 19.0f
			? 0.0f
			: 138.5177312231f * std::log(temperature - 10.0f) - 305.0447927307f;
	} else {
		red = 329.698727446f * std::pow(temperature - 60.0f, -0.1332047592f);
		green = 288.1221695283f * std::pow(temperature - 60.0f, -0.0755148492f);
	}
	const glm::vec3 srgb(
		std::clamp(red / 255.0f, 0.0f, 1.0f),
		std::clamp(green / 255.0f, 0.0f, 1.0f),
		std::clamp(blue / 255.0f, 0.0f, 1.0f));
	return glm::vec3(
		srgb_to_linear(srgb.r), srgb_to_linear(srgb.g), srgb_to_linear(srgb.b));
}
