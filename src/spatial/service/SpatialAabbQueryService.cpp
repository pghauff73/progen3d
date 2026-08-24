#include "spatial/service/SpatialAabbQueryService.h"

#include <array>
#include <cmath>
#include <limits>

namespace {

AxisAlignedBounds bounds_from_minimum_and_maximum(const glm::vec3 &minimum,
	                                               const glm::vec3 &maximum)
{
	AxisAlignedBounds bounds;
	bounds.min = minimum;
	bounds.max = maximum;
	bounds.center = (minimum + maximum) * 0.5f;
	bounds.half_extents = (maximum - minimum) * 0.5f;
	bounds.valid = true;
	return bounds;
}

} // namespace

AxisAlignedBounds SpatialAabbQueryService::transform(
	const AxisAlignedBounds &bounds,
	const glm::mat4 &transform_matrix) const
{
	if (!bounds.valid) return {};
	const std::array<glm::vec3, 8> corners = {
		glm::vec3(bounds.min.x, bounds.min.y, bounds.min.z),
		glm::vec3(bounds.max.x, bounds.min.y, bounds.min.z),
		glm::vec3(bounds.min.x, bounds.max.y, bounds.min.z),
		glm::vec3(bounds.max.x, bounds.max.y, bounds.min.z),
		glm::vec3(bounds.min.x, bounds.min.y, bounds.max.z),
		glm::vec3(bounds.max.x, bounds.min.y, bounds.max.z),
		glm::vec3(bounds.min.x, bounds.max.y, bounds.max.z),
		glm::vec3(bounds.max.x, bounds.max.y, bounds.max.z)};
	glm::vec3 minimum(std::numeric_limits<float>::max());
	glm::vec3 maximum(std::numeric_limits<float>::lowest());
	for (const glm::vec3 &corner : corners) {
		const glm::vec4 transformed = transform_matrix * glm::vec4(corner, 1.0f);
		if (!std::isfinite(transformed.x) || !std::isfinite(transformed.y) ||
		    !std::isfinite(transformed.z) || !std::isfinite(transformed.w) ||
		    std::fabs(transformed.w) <= 1.0e-7f) {
			return {};
		}
		const glm::vec3 point = glm::vec3(transformed) / transformed.w;
		minimum = glm::min(minimum, point);
		maximum = glm::max(maximum, point);
	}
	return bounds_from_minimum_and_maximum(minimum, maximum);
}

AxisAlignedBounds SpatialAabbQueryService::translated(
	const AxisAlignedBounds &bounds,
	const glm::vec3 &translation) const
{
	if (!bounds.valid) return {};
	return bounds_from_minimum_and_maximum(bounds.min + translation,
	                                      bounds.max + translation);
}

AxisAlignedBounds SpatialAabbQueryService::merged(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second) const
{
	if (!first.valid) return second;
	if (!second.valid) return first;
	return bounds_from_minimum_and_maximum(
		glm::min(first.min, second.min), glm::max(first.max, second.max));
}

bool SpatialAabbQueryService::overlaps(const AxisAlignedBounds &first,
	                                   const AxisAlignedBounds &second,
	                                   float tolerance) const
{
	if (!first.valid || !second.valid) return false;
	return first.max.x > second.min.x + tolerance &&
	       second.max.x > first.min.x + tolerance &&
	       first.max.y > second.min.y + tolerance &&
	       second.max.y > first.min.y + tolerance &&
	       first.max.z > second.min.z + tolerance &&
	       second.max.z > first.min.z + tolerance;
}

bool SpatialAabbQueryService::touchesOrOverlaps(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second,
	float tolerance) const
{
	if (!first.valid || !second.valid) return false;
	return first.max.x >= second.min.x - tolerance &&
	       second.max.x >= first.min.x - tolerance &&
	       first.max.y >= second.min.y - tolerance &&
	       second.max.y >= first.min.y - tolerance &&
	       first.max.z >= second.min.z - tolerance &&
	       second.max.z >= first.min.z - tolerance;
}

float SpatialAabbQueryService::projectedExtent(
	const AxisAlignedBounds &bounds,
	const glm::vec3 &unit_direction) const
{
	if (!bounds.valid) return 0.0f;
	const glm::vec3 full_extents = bounds.half_extents * 2.0f;
	const glm::vec3 absolute_direction = glm::abs(unit_direction);
	return glm::dot(full_extents, absolute_direction);
}
