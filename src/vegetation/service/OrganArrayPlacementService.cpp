#include "vegetation/service/OrganArrayPlacementService.h"

#include "vegetation/service/DeterministicPlantVariationService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool supported_organ_type(VegetationOrganType organ_type)
{
	return organ_type == VegetationOrganType::Bud ||
	       organ_type == VegetationOrganType::Leaf ||
	       organ_type == VegetationOrganType::Petal ||
	       organ_type == VegetationOrganType::Flower ||
	       organ_type == VegetationOrganType::Fruit ||
	       organ_type == VegetationOrganType::Thorn;
}

bool valid_specification(
	const PlantOrganArraySpecification &specification,
	std::string *diagnostic)
{
	if (specification.identifier().empty() ||
	    !supported_organ_type(specification.organType()) ||
	    specification.count() < 1 ||
	    !std::isfinite(specification.spacing()) ||
	    specification.spacing() < 0.0f ||
	    !std::isfinite(specification.azimuthProgressionDegrees()) ||
	    !std::isfinite(specification.initialScale()) ||
	    specification.initialScale() <= 0.0f ||
	    !std::isfinite(specification.scaleFalloff()) ||
	    specification.scaleFalloff() <= 0.0f ||
	    !std::isfinite(specification.jitterFraction()) ||
	    specification.jitterFraction() < 0.0f ||
	    specification.jitterFraction() > 1.0f ||
	    specification.minimumBranchOrder() < 0) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"OrganArray requires a supported organ, positive count and scale, finite spacing and azimuth, jitter in [0,1], and a non-negative branch order.";
		}
		return false;
	}
	return true;
}

glm::vec3 perpendicular_axis(const glm::vec3 &direction)
{
	const glm::vec3 reference = std::abs(direction.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(reference, direction));
}

glm::mat4 frame_transform(
	const glm::vec3 &origin,
	const glm::vec3 &local_y,
	const glm::vec3 &normal_hint,
	float uniform_scale)
{
	glm::vec3 local_x = glm::cross(normal_hint, local_y);
	if (glm::length(local_x) <= 1.0e-6f) {
		local_x = perpendicular_axis(local_y);
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
	return glm::scale(transform, glm::vec3(uniform_scale));
}

struct PathSample
{
	glm::vec3 position{0.0f};
	glm::vec3 tangent{0.0f, 1.0f, 0.0f};
};

float path_length(const std::vector<glm::vec3> &path_points)
{
	float length = 0.0f;
	for (std::size_t index = 1u; index < path_points.size(); ++index) {
		length += glm::length(path_points[index] - path_points[index - 1u]);
	}
	return length;
}

PathSample sample_path(
	const std::vector<glm::vec3> &path_points,
	float distance)
{
	float traversed = 0.0f;
	for (std::size_t index = 0u; index + 1u < path_points.size(); ++index) {
		const glm::vec3 segment = path_points[index + 1u] - path_points[index];
		const float segment_length = glm::length(segment);
		if (distance <= traversed + segment_length ||
		    index + 2u == path_points.size()) {
			const float local_distance = segment_length <= 1.0e-6f
				? 0.0f
				: std::clamp(
					(distance - traversed) / segment_length, 0.0f, 1.0f);
			return {
				glm::mix(path_points[index], path_points[index + 1u], local_distance),
				glm::normalize(segment)};
		}
		traversed += segment_length;
	}
	return {
		path_points.back(),
		glm::normalize(path_points.back() - path_points[path_points.size() - 2u])};
}

float placement_scale(
	const PlantOrganArraySpecification &specification,
	std::size_t sequence_index,
	std::uint64_t deterministic_seed,
	const std::string &scope)
{
	const float falloff = std::pow(
		specification.scaleFalloff(), static_cast<float>(sequence_index));
	const float variation = 1.0f +
		0.25f * specification.jitterFraction() *
		DeterministicPlantVariationService().sampleSigned(
			deterministic_seed, scope + ":scale");
	return specification.initialScale() * falloff * variation;
}

float placement_azimuth(
	const PlantOrganArraySpecification &specification,
	std::size_t sequence_index,
	std::uint64_t deterministic_seed,
	const std::string &scope)
{
	const float jitter_span = std::max(
		std::abs(specification.azimuthProgressionDegrees()), 45.0f);
	return static_cast<float>(sequence_index) *
			specification.azimuthProgressionDegrees() +
		0.5f * jitter_span * specification.jitterFraction() *
		DeterministicPlantVariationService().sampleSigned(
			deterministic_seed, scope + ":azimuth");
}

glm::vec3 oriented_direction(
	VegetationOrganOrientation orientation,
	const glm::vec3 &outward,
	const glm::vec3 &host_direction)
{
	switch (orientation) {
	case VegetationOrganOrientation::Outward:
		return outward;
	case VegetationOrganOrientation::AlongHost:
		return host_direction;
	case VegetationOrganOrientation::WorldUp:
		return glm::vec3(0.0f, 1.0f, 0.0f);
	case VegetationOrganOrientation::WorldDown:
		return glm::vec3(0.0f, -1.0f, 0.0f);
	case VegetationOrganOrientation::SurfaceNormal:
		return outward;
	}
	return outward;
}

} // namespace

std::vector<OrganArrayPlacement> OrganArrayPlacementService::placeAlongPath(
	const PlantOrganArraySpecification &specification,
	const std::vector<glm::vec3> &path_points,
	std::uint64_t deterministic_seed,
	std::string *diagnostic) const
{
	if (!valid_specification(specification, diagnostic)) return {};
	if (specification.host() != VegetationOrganArrayHost::Stem &&
	    specification.host() != VegetationOrganArrayHost::Branch) {
		if (diagnostic != nullptr) {
			*diagnostic = "Path OrganArray placement requires a Stem or Branch host.";
		}
		return {};
	}
	if (specification.orientation() ==
	    VegetationOrganOrientation::SurfaceNormal) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"SurfaceNormal orientation requires an OrganArray Surface host.";
		}
		return {};
	}
	if (path_points.size() < 2u) {
		if (diagnostic != nullptr) {
			*diagnostic = "Path OrganArray placement requires at least two points.";
		}
		return {};
	}
	for (std::size_t index = 0u; index < path_points.size(); ++index) {
		if (!finite(path_points[index]) ||
		    (index > 0u && glm::length(
			 path_points[index] - path_points[index - 1u]) <= 1.0e-6f)) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Path OrganArray placement requires finite, distinct path points.";
			}
			return {};
		}
	}

	const float total_length = path_length(path_points);
	const float requested_length = static_cast<float>(specification.count() - 1) *
	                               specification.spacing();
	if (requested_length > total_length + 1.0e-5f) {
		if (diagnostic != nullptr) {
			*diagnostic = "OrganArray spacing extends beyond the host path.";
		}
		return {};
	}

	std::vector<OrganArrayPlacement> placements;
	placements.reserve(static_cast<std::size_t>(specification.count()));
	for (int index = 0; index < specification.count(); ++index) {
		const std::string scope = specification.identifier() + ":" +
		                          std::to_string(index);
		const float nominal_distance = static_cast<float>(index) *
		                               specification.spacing();
		const float distance_jitter = specification.spacing() *
			0.45f * specification.jitterFraction() *
			DeterministicPlantVariationService().sampleSigned(
				deterministic_seed, scope + ":distance");
		const float host_distance = std::clamp(
			nominal_distance + distance_jitter, 0.0f, total_length);
		const PathSample sample = sample_path(path_points, host_distance);
		const glm::vec3 radial_x = perpendicular_axis(sample.tangent);
		const glm::vec3 radial_z = glm::normalize(
			glm::cross(sample.tangent, radial_x));
		const float azimuth = placement_azimuth(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		const float angle = glm::radians(azimuth);
		const glm::vec3 outward = glm::normalize(
			radial_x * std::cos(angle) + radial_z * std::sin(angle));
		const glm::vec3 direction = glm::normalize(oriented_direction(
			specification.orientation(), outward, sample.tangent));
		const float scale = placement_scale(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		placements.emplace_back(
			scope, specification.organType(), specification.host(),
			static_cast<std::size_t>(index), host_distance, azimuth, scale,
			sample.position, direction,
			frame_transform(sample.position, direction, sample.tangent, scale));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}

std::vector<OrganArrayPlacement>
OrganArrayPlacementService::placeAroundFlowerHead(
	const PlantOrganArraySpecification &specification,
	glm::vec3 origin,
	glm::vec3 axis,
	std::uint64_t deterministic_seed,
	std::string *diagnostic) const
{
	if (!valid_specification(specification, diagnostic)) return {};
	if (specification.host() != VegetationOrganArrayHost::FlowerHead) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Flower-head OrganArray placement requires a FlowerHead host.";
		}
		return {};
	}
	if (!finite(origin) || !finite(axis) || glm::length(axis) <= 1.0e-6f ||
	    specification.orientation() ==
		    VegetationOrganOrientation::SurfaceNormal) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Flower-head OrganArray placement requires a finite axis and a non-surface orientation.";
		}
		return {};
	}
	axis = glm::normalize(axis);
	const glm::vec3 radial_x = perpendicular_axis(axis);
	const glm::vec3 radial_z = glm::normalize(glm::cross(axis, radial_x));
	std::vector<OrganArrayPlacement> placements;
	placements.reserve(static_cast<std::size_t>(specification.count()));
	for (int index = 0; index < specification.count(); ++index) {
		const std::string scope = specification.identifier() + ":" +
		                          std::to_string(index);
		const float azimuth = placement_azimuth(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		const float angle = glm::radians(azimuth);
		const glm::vec3 outward = glm::normalize(
			radial_x * std::cos(angle) + radial_z * std::sin(angle));
		const float radial_jitter = specification.spacing() * 0.25f *
			specification.jitterFraction() *
			DeterministicPlantVariationService().sampleSigned(
				deterministic_seed, scope + ":radius");
		const glm::vec3 position = origin +
			std::max(0.0f, specification.spacing() + radial_jitter) * outward;
		const glm::vec3 direction = glm::normalize(oriented_direction(
			specification.orientation(), outward, axis));
		const float scale = placement_scale(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		placements.emplace_back(
			scope, specification.organType(), specification.host(),
			static_cast<std::size_t>(index), 0.0f, azimuth, scale, position,
			direction, frame_transform(position, direction, axis, scale));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}

std::vector<OrganArrayPlacement> OrganArrayPlacementService::placeOnSurface(
	const PlantOrganArraySpecification &specification,
	const std::vector<SurfaceAttachmentPoint> &surface_samples,
	std::uint64_t deterministic_seed,
	std::string *diagnostic) const
{
	if (!valid_specification(specification, diagnostic)) return {};
	if (specification.host() != VegetationOrganArrayHost::Surface) {
		if (diagnostic != nullptr) {
			*diagnostic = "Surface OrganArray placement requires a Surface host.";
		}
		return {};
	}
	if (surface_samples.empty() ||
	    static_cast<std::size_t>(specification.count()) > surface_samples.size()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Surface OrganArray placement requires at least one finite sample per organ.";
		}
		return {};
	}

	std::vector<OrganArrayPlacement> placements;
	placements.reserve(static_cast<std::size_t>(specification.count()));
	for (int index = 0; index < specification.count(); ++index) {
		const std::size_t sample_index = specification.count() == 1
			? 0u
			: static_cast<std::size_t>(std::llround(
				static_cast<double>(index) *
				static_cast<double>(surface_samples.size() - 1u) /
				static_cast<double>(specification.count() - 1)));
		const SurfaceAttachmentPoint &sample = surface_samples[sample_index];
		if (!finite(sample.position()) || !finite(sample.normal()) ||
		    glm::length(sample.normal()) <= 1.0e-6f) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Surface OrganArray placement requires finite sample positions and normals.";
			}
			return {};
		}
		const glm::vec3 normal = glm::normalize(sample.normal());
		const glm::vec3 tangent_x = perpendicular_axis(normal);
		const glm::vec3 tangent_z = glm::normalize(glm::cross(normal, tangent_x));
		const std::string scope = specification.identifier() + ":" +
		                          std::to_string(index);
		const float azimuth = placement_azimuth(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		const float angle = glm::radians(azimuth);
		const glm::vec3 tangent = glm::normalize(
			tangent_x * std::cos(angle) + tangent_z * std::sin(angle));
		const float tangent_jitter = specification.spacing() *
			specification.jitterFraction() *
			DeterministicPlantVariationService().sampleSigned(
				deterministic_seed, scope + ":surface");
		const glm::vec3 position = sample.position() + tangent_jitter * tangent;
		const glm::vec3 direction = glm::normalize(oriented_direction(
			specification.orientation(), normal, tangent));
		const float scale = placement_scale(
			specification, static_cast<std::size_t>(index), deterministic_seed,
			scope);
		placements.emplace_back(
			scope, specification.organType(), specification.host(),
			static_cast<std::size_t>(index), sample.distance(), azimuth, scale,
			position, direction,
			frame_transform(position, direction, normal, scale));
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return placements;
}
