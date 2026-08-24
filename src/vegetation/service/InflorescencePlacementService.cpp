#include "vegetation/service/InflorescencePlacementService.h"

#include "vegetation/model/FlowerHeadSpecification.h"
#include "vegetation/service/FlowerHeadPlacementService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

glm::vec3 perpendicular_axis(const glm::vec3 &axis)
{
	const glm::vec3 reference = std::abs(axis.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(reference, axis));
}

glm::mat4 placement_transform(
	const glm::vec3 &position,
	const glm::vec3 &direction,
	const glm::vec3 &normal_hint,
	float scale)
{
	glm::vec3 local_x = glm::cross(normal_hint, direction);
	if (glm::length(local_x) <= 1.0e-6f) {
		local_x = perpendicular_axis(direction);
	}
	else {
		local_x = glm::normalize(local_x);
	}
	const glm::vec3 local_z = glm::normalize(glm::cross(local_x, direction));
	glm::mat4 transform(1.0f);
	transform[0] = glm::vec4(local_x, 0.0f);
	transform[1] = glm::vec4(direction, 0.0f);
	transform[2] = glm::vec4(local_z, 0.0f);
	transform[3] = glm::vec4(position, 1.0f);
	return glm::scale(transform, glm::vec3(scale));
}

} // namespace

std::vector<InflorescencePlacement> InflorescencePlacementService::place(
	const InflorescenceSpecification &specification,
	const std::string &identifier_prefix,
	glm::vec3 origin,
	glm::vec3 axis,
	std::string *diagnostic) const
{
	if (!specification.isEnabled() || identifier_prefix.empty() || !finite(origin) ||
	    !finite(axis) || glm::length(axis) <= 1.0e-6f ||
	    specification.flowerCount() < 1 ||
	    static_cast<std::size_t>(specification.flowerCount()) >
		    complexity_limits_.maximumOrganAttachments() ||
	    !std::isfinite(specification.spacing()) || specification.spacing() < 0.0f ||
	    !std::isfinite(specification.radialExtent()) ||
	    specification.radialExtent() < 0.0f ||
	    !std::isfinite(specification.phaseDegrees()) ||
	    !std::isfinite(specification.tiltDegrees()) ||
	    !std::isfinite(specification.scaleFalloff()) ||
	    specification.scaleFalloff() <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Inflorescence requires a finite frame, positive bounded flower count, non-negative spacing and extent, finite angles, and positive scale falloff.";
		}
		return {};
	}
	if (specification.kind() == InflorescenceKind::Head) {
		return FlowerHeadPlacementService(complexity_limits_).place(
			FlowerHeadSpecification(
				specification.flowerCount(), specification.radialExtent(),
				137.50776f, specification.phaseDegrees(),
				specification.tiltDegrees(), specification.scaleFalloff()),
			identifier_prefix, origin, axis, diagnostic);
	}

	axis = glm::normalize(axis);
	const glm::vec3 tangent_x = perpendicular_axis(axis);
	const glm::vec3 tangent_z = glm::normalize(glm::cross(axis, tangent_x));
	const float tilt = glm::radians(specification.tiltDegrees());
	std::vector<InflorescencePlacement> placements;
	placements.reserve(static_cast<std::size_t>(specification.flowerCount()));
	for (int index = 0; index < specification.flowerCount(); ++index) {
		const float fraction = specification.flowerCount() == 1
			? 0.0f
			: static_cast<float>(index) /
			  static_cast<float>(specification.flowerCount() - 1);
		const float azimuth = glm::radians(
			specification.phaseDegrees() + 137.50776f * static_cast<float>(index));
		const glm::vec3 radial_direction = glm::normalize(
			tangent_x * std::cos(azimuth) + tangent_z * std::sin(azimuth));
		float axial_distance = specification.spacing() * static_cast<float>(index);
		float radial_distance = specification.radialExtent();
		switch (specification.kind()) {
		case InflorescenceKind::Spike:
			radial_distance = 0.0f;
			break;
		case InflorescenceKind::Panicle:
			radial_distance *= 1.0f - 0.65f * fraction;
			break;
		case InflorescenceKind::Umbel:
			axial_distance = specification.spacing();
			break;
		case InflorescenceKind::Corymb:
			axial_distance = specification.spacing() *
				static_cast<float>(specification.flowerCount() - 1);
			break;
		case InflorescenceKind::Raceme:
		case InflorescenceKind::Head:
			break;
		}
		const glm::vec3 position =
			origin + axis * axial_distance + radial_direction * radial_distance;
		const glm::vec3 direction = glm::normalize(
			axis * std::cos(tilt) + radial_direction * std::sin(tilt));
		const float scale = std::pow(
			specification.scaleFalloff(), static_cast<float>(index));
		const std::string identifier =
			identifier_prefix + ":flower:" + std::to_string(index);
		placements.emplace_back(
			identifier, specification.kind(), static_cast<std::size_t>(index),
			position, direction, scale,
			placement_transform(position, direction, axis, scale));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}
