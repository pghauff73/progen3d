#include "lighting/model/SceneLight.h"

#include <algorithm>
#include <cmath>

void SceneLight::setType(SceneLightType type)
{
	type_ = type;
	switch (type_) {
	case SceneLightType::Directional:
		data_ = DirectionalLightData{};
		break;
	case SceneLightType::Point:
		data_ = PointLightData{};
		break;
	case SceneLightType::Spot:
		data_ = SpotLightData{};
		break;
	case SceneLightType::RectangularArea:
		data_ = RectangularAreaLightData{};
		break;
	}
}

glm::vec3 SceneLight::emissionDirection() const
{
	if (const auto *directional = std::get_if<DirectionalLightData>(&data_)) {
		if (glm::length(directional->direction) > 0.0001f) {
			return glm::normalize(directional->direction);
		}
	}
	const glm::quat normalized_orientation = glm::normalize(orientation_);
	const glm::vec3 quaternion_axis(
		normalized_orientation.x, normalized_orientation.y, normalized_orientation.z);
	const glm::vec3 local_direction(0.0f, -1.0f, 0.0f);
	const glm::vec3 direction = local_direction +
		2.0f * glm::cross(
			quaternion_axis,
			glm::cross(quaternion_axis, local_direction) +
				normalized_orientation.w * local_direction);
	return glm::length(direction) > 0.0001f
		? glm::normalize(direction)
		: glm::vec3(0.0f, -1.0f, 0.0f);
}

void SceneLight::setEmissionDirection(const glm::vec3 &direction)
{
	if (glm::length(direction) <= 0.0001f) return;
	const glm::vec3 normalized_direction = glm::normalize(direction);
	if (auto *directional = std::get_if<DirectionalLightData>(&data_)) {
		directional->direction = normalized_direction;
		return;
	}
	const glm::vec3 local_direction(0.0f, -1.0f, 0.0f);
	const float cosine = std::clamp(glm::dot(local_direction, normalized_direction), -1.0f, 1.0f);
	if (cosine < -0.9999f) {
		orientation_ = glm::quat(0.0f, 1.0f, 0.0f, 0.0f);
		return;
	}
	const glm::vec3 axis = glm::cross(local_direction, normalized_direction);
	const float scale = std::sqrt((1.0f + cosine) * 2.0f);
	if (scale <= 0.0001f) {
		orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		return;
	}
	orientation_ = glm::normalize(glm::quat(
		scale * 0.5f, axis.x / scale, axis.y / scale, axis.z / scale));
}

float SceneLight::range() const
{
	if (const auto *point = std::get_if<PointLightData>(&data_)) return point->range;
	if (const auto *spot = std::get_if<SpotLightData>(&data_)) return spot->range;
	if (const auto *area = std::get_if<RectangularAreaLightData>(&data_)) return area->range;
	return 0.0f;
}
