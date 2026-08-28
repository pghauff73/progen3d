#include "vegetation/service/OrganPlacementService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

glm::vec3 fallback_axis(const glm::vec3 &direction)
{
	const glm::vec3 reference = std::abs(direction.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(reference, direction));
}

glm::mat4 frame_transform(
	const glm::vec3 &origin,
	const glm::vec3 &local_y,
	const glm::vec3 &normal_hint)
{
	glm::vec3 local_x = glm::cross(normal_hint, local_y);
	if (glm::length(local_x) <= 1.0e-6f) {
		local_x = fallback_axis(local_y);
	}
	else {
		local_x = glm::normalize(local_x);
	}
	const glm::vec3 local_z = glm::normalize(glm::cross(local_x, local_y));
	glm::mat4 transform(1.0f);
	transform[0] = glm::vec4(local_x, 0.0f);
	transform[1] = glm::vec4(local_y, 0.0f);
	transform[2] = glm::vec4(local_z, 0.0f);
	transform[3] = glm::vec4(origin, 1.0f);
	return transform;
}

struct PathSample
{
	glm::vec3 position{0.0f};
	glm::vec3 tangent{0.0f, 1.0f, 0.0f};
};

PathSample sample_path(
	const std::vector<glm::vec3> &path_points,
	float distance)
{
	float traversed = 0.0f;
	for (std::size_t index = 0; index + 1u < path_points.size(); ++index) {
		const glm::vec3 segment = path_points[index + 1u] - path_points[index];
		const float length = glm::length(segment);
		if (distance <= traversed + length || index + 2u == path_points.size()) {
			const float local = length <= 1.0e-6f
				? 0.0f
				: std::max(0.0f, std::min(1.0f, (distance - traversed) / length));
			return {
				glm::mix(path_points[index], path_points[index + 1u], local),
				glm::normalize(segment)};
		}
		traversed += length;
	}
	return {path_points.back(),
	        glm::normalize(path_points.back() - path_points[path_points.size() - 2u])};
}

std::size_t organs_at_node(
	PhyllotaxisMode mode,
	int configured_organs_per_node)
{
	switch (mode) {
	case PhyllotaxisMode::Opposite:
	case PhyllotaxisMode::Decussate:
		return 2u;
	case PhyllotaxisMode::Whorled:
		return static_cast<std::size_t>(configured_organs_per_node);
	default:
		return 1u;
	}
}

float node_phase(
	const PhyllotaxisSpecification &specification,
	std::size_t node_index)
{
	switch (specification.mode()) {
	case PhyllotaxisMode::Decussate:
		return specification.phaseDegrees() +
		       (node_index % 2u == 0u ? 0.0f : 90.0f);
	case PhyllotaxisMode::Alternate:
	case PhyllotaxisMode::Spiral:
		return specification.phaseDegrees() +
		       static_cast<float>(node_index) * specification.divergenceDegrees();
	case PhyllotaxisMode::Rosette:
		return specification.phaseDegrees() +
		       static_cast<float>(node_index) * specification.divergenceDegrees();
	default:
		return specification.phaseDegrees();
	}
}

} // namespace

std::vector<VegetationOrganPlacement>
OrganPlacementService::placeAlongPath(
	const std::string &identifier_prefix,
	const std::vector<glm::vec3> &path_points,
	std::size_t node_count,
	const PhyllotaxisSpecification &specification,
	std::string *diagnostic) const
{
	if (identifier_prefix.empty() || path_points.size() < 2u || node_count == 0u ||
	    !std::isfinite(specification.divergenceDegrees()) ||
	    !std::isfinite(specification.internodeLength()) ||
	    specification.internodeLength() < 0.0f ||
	    specification.organsPerNode() < 1 ||
	    !std::isfinite(specification.phaseDegrees()) ||
	    !std::isfinite(specification.radialOffset()) ||
	    specification.radialOffset() < 0.0f ||
	    !std::isfinite(specification.orientationUpBias())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Organ placement requires a valid identifier, path, count, and finite phyllotaxis parameters.";
		}
		return {};
	}
	float total_length = 0.0f;
	for (std::size_t index = 0; index < path_points.size(); ++index) {
		if (!finite(path_points[index])) {
			if (diagnostic != nullptr) {
				*diagnostic = "Organ placement path points must be finite.";
			}
			return {};
		}
		if (index > 0u) {
			const float length = glm::length(path_points[index] - path_points[index - 1u]);
			if (length <= 1.0e-6f) {
				if (diagnostic != nullptr) {
					*diagnostic =
						"Organ placement path requires distinct consecutive points.";
				}
				return {};
			}
			total_length += length;
		}
	}
	const float requested_length = specification.mode() == PhyllotaxisMode::Rosette
		? 0.0f
		: static_cast<float>(node_count - 1u) * specification.internodeLength();
	if (requested_length > total_length + 1.0e-5f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Phyllotaxis internodes extend beyond the host path.";
		}
		return {};
	}

	std::vector<VegetationOrganPlacement> placements;
	const std::size_t per_node = organs_at_node(
		specification.mode(), specification.organsPerNode());
	placements.reserve(node_count * per_node);
	for (std::size_t node_index = 0; node_index < node_count; ++node_index) {
		const float path_distance = specification.mode() == PhyllotaxisMode::Rosette
			? 0.0f
			: static_cast<float>(node_index) * specification.internodeLength();
		const PathSample sample = sample_path(path_points, path_distance);
		const glm::vec3 radial_x = fallback_axis(sample.tangent);
		const glm::vec3 radial_y = glm::normalize(glm::cross(sample.tangent, radial_x));
		const float base_phase = node_phase(specification, node_index);
		for (std::size_t organ_index = 0; organ_index < per_node; ++organ_index) {
			const float azimuth = base_phase +
				360.0f * static_cast<float>(organ_index) /
				static_cast<float>(per_node);
			const float angle = glm::radians(azimuth);
			const glm::vec3 radial_direction =
				radial_x * std::cos(angle) + radial_y * std::sin(angle);
			const glm::vec3 direction = glm::normalize(
				radial_direction +
				specification.orientationUpBias() * sample.tangent);
			const glm::vec3 position = sample.position +
				specification.radialOffset() * radial_direction;
			placements.emplace_back(
				identifier_prefix + "_" + std::to_string(node_index) + "_" +
					std::to_string(organ_index),
				node_index, organ_index, path_distance, azimuth, position,
				direction, frame_transform(position, direction, sample.tangent));
		}
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}

std::vector<VegetationOrganPlacement> OrganPlacementService::placeWhorl(
	const std::string &identifier_prefix,
	glm::vec3 origin,
	glm::vec3 axis,
	const WhorlSpecification &specification,
	std::string *diagnostic) const
{
	if (identifier_prefix.empty() || !finite(origin) || !finite(axis) ||
	    glm::length(axis) <= 1.0e-6f || specification.organCount() < 1 ||
	    !std::isfinite(specification.radius()) || specification.radius() < 0.0f ||
	    !std::isfinite(specification.phaseDegrees()) ||
	    !std::isfinite(specification.tiltDegrees())) {
		if (diagnostic != nullptr) {
			*diagnostic = "Whorl placement requires finite geometry and a positive organ count.";
		}
		return {};
	}
	axis = glm::normalize(axis);
	const glm::vec3 radial_x = fallback_axis(axis);
	const glm::vec3 radial_y = glm::normalize(glm::cross(axis, radial_x));
	const float tilt = glm::radians(specification.tiltDegrees());
	std::vector<VegetationOrganPlacement> placements;
	placements.reserve(static_cast<std::size_t>(specification.organCount()));
	for (int index = 0; index < specification.organCount(); ++index) {
		const float azimuth = specification.phaseDegrees() +
			360.0f * static_cast<float>(index) /
			static_cast<float>(specification.organCount());
		const float angle = glm::radians(azimuth);
		const glm::vec3 radial_direction =
			radial_x * std::cos(angle) + radial_y * std::sin(angle);
		const glm::vec3 direction = glm::normalize(
			radial_direction * std::cos(tilt) + axis * std::sin(tilt));
		const glm::vec3 position = origin + specification.radius() * radial_direction;
		placements.emplace_back(
			identifier_prefix + "_" + std::to_string(index), 0u,
			static_cast<std::size_t>(index), 0.0f, azimuth, position, direction,
			frame_transform(position, direction, axis));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}

std::vector<VegetationOrganPlacement> OrganPlacementService::placeAtNode(
	const std::string &identifier_prefix,
	std::size_t node_index,
	glm::vec3 origin,
	glm::vec3 axis,
	const PhyllotaxisSpecification &specification,
	std::string *diagnostic) const
{
	if (identifier_prefix.empty() || !finite(origin) || !finite(axis) ||
	    glm::length(axis) <= 1.0e-6f ||
	    !std::isfinite(specification.divergenceDegrees()) ||
	    !std::isfinite(specification.internodeLength()) ||
	    specification.internodeLength() < 0.0f ||
	    specification.organsPerNode() < 1 ||
	    !std::isfinite(specification.phaseDegrees()) ||
	    !std::isfinite(specification.radialOffset()) ||
	    specification.radialOffset() < 0.0f ||
	    !std::isfinite(specification.orientationUpBias())) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Node organ placement requires a finite node frame and phyllotaxis parameters.";
		}
		return {};
	}

	axis = glm::normalize(axis);
	const glm::vec3 radial_x = fallback_axis(axis);
	const glm::vec3 radial_y = glm::normalize(glm::cross(axis, radial_x));
	const std::size_t per_node = organs_at_node(
		specification.mode(), specification.organsPerNode());
	const float base_phase = node_phase(specification, node_index);
	std::vector<VegetationOrganPlacement> placements;
	placements.reserve(per_node);
	for (std::size_t organ_index = 0u; organ_index < per_node; ++organ_index) {
		const float azimuth = base_phase +
			360.0f * static_cast<float>(organ_index) /
			static_cast<float>(per_node);
		const float angle = glm::radians(azimuth);
		const glm::vec3 radial_direction =
			radial_x * std::cos(angle) + radial_y * std::sin(angle);
		const glm::vec3 direction = glm::normalize(
			radial_direction + specification.orientationUpBias() * axis);
		const glm::vec3 position = origin +
			specification.radialOffset() * radial_direction;
		placements.emplace_back(
			identifier_prefix + "_" + std::to_string(node_index) + "_" +
				std::to_string(organ_index),
			node_index, organ_index, 0.0f, azimuth, position, direction,
			frame_transform(position, direction, axis));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}
