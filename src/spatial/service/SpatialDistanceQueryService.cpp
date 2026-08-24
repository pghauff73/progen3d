#include "spatial/service/SpatialDistanceQueryService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {

struct AxisDistanceEvidence {
	float first_point = 0.0f;
	float second_point = 0.0f;
	float separation = 0.0f;
	float overlap = 0.0f;
};

AxisDistanceEvidence measure_axis(float first_minimum,
	                              float first_maximum,
	                              float second_minimum,
	                              float second_maximum)
{
	if (first_maximum < second_minimum) {
		return {first_maximum, second_minimum, second_minimum - first_maximum, 0.0f};
	}
	if (second_maximum < first_minimum) {
		return {first_minimum, second_maximum, first_minimum - second_maximum, 0.0f};
	}
	const float overlap_minimum = std::max(first_minimum, second_minimum);
	const float overlap_maximum = std::min(first_maximum, second_maximum);
	const float midpoint = (overlap_minimum + overlap_maximum) * 0.5f;
	return {midpoint, midpoint, 0.0f, overlap_maximum - overlap_minimum};
}

} // namespace

SpatialRelationResult SpatialDistanceQueryService::measure(
	const SpatialObjectId &first_object_id,
	const AxisAlignedBounds &first,
	const SpatialObjectId &second_object_id,
	const AxisAlignedBounds &second,
	float tolerance) const
{
	if (!first.valid || !second.valid) {
		return SpatialRelationResult(
			first_object_id, second_object_id, SpatialRelationKind::Separated,
			std::numeric_limits<float>::infinity(), 0.0f, glm::vec3(0.0f),
			glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f);
	}
	const std::array<AxisDistanceEvidence, 3> axes = {
		measure_axis(first.min.x, first.max.x, second.min.x, second.max.x),
		measure_axis(first.min.y, first.max.y, second.min.y, second.max.y),
		measure_axis(first.min.z, first.max.z, second.min.z, second.max.z)};
	const glm::vec3 first_point(
		axes[0].first_point, axes[1].first_point, axes[2].first_point);
	const glm::vec3 second_point(
		axes[0].second_point, axes[1].second_point, axes[2].second_point);
	const glm::vec3 separation_vector = second_point - first_point;
	const float distance = glm::length(separation_vector);
	const bool separated = axes[0].separation > tolerance ||
	                       axes[1].separation > tolerance ||
	                       axes[2].separation > tolerance;
	if (separated) {
		const glm::vec3 normal = distance > tolerance
			? separation_vector / distance
			: glm::vec3(0.0f);
		return SpatialRelationResult(
			first_object_id, second_object_id, SpatialRelationKind::Separated,
			distance, 0.0f, (first_point + second_point) * 0.5f, normal,
			first_point, second_point, 0.5f);
	}

	const float minimum_overlap = std::min(
		axes[0].overlap, std::min(axes[1].overlap, axes[2].overlap));
	const SpatialRelationKind relation = minimum_overlap <= tolerance
		? SpatialRelationKind::Touching
		: SpatialRelationKind::Overlapping;
	glm::vec3 normal(0.0f);
	if (relation == SpatialRelationKind::Overlapping) {
		int axis = 0;
		if (axes[1].overlap < axes[axis].overlap) axis = 1;
		if (axes[2].overlap < axes[axis].overlap) axis = 2;
		normal[axis] = second.center[axis] >= first.center[axis] ? 1.0f : -1.0f;
	}
	return SpatialRelationResult(
		first_object_id, second_object_id, relation, 0.0f,
		std::max(0.0f, minimum_overlap), (first_point + second_point) * 0.5f,
		normal, first_point, second_point, 0.5f);
}
