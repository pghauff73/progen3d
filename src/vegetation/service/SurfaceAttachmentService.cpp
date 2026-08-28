#include "vegetation/service/SurfaceAttachmentService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace {

constexpr float kDirectionTolerance = 1.0e-8f;

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x &&
	       bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
}

bool point_inside_bounds(const glm::vec3 &point, const AxisAlignedBounds &bounds)
{
	return point.x >= bounds.min.x && point.x <= bounds.max.x &&
	       point.y >= bounds.min.y && point.y <= bounds.max.y &&
	       point.z >= bounds.min.z && point.z <= bounds.max.z;
}

glm::vec3 axis_normal(int axis, float sign)
{
	glm::vec3 normal(0.0f);
	normal[axis] = sign;
	return normal;
}

struct NearestSurfacePoint
{
	glm::vec3 point{0.0f};
	glm::vec3 normal{0.0f};
};

NearestSurfacePoint nearest_surface_point(
	const glm::vec3 &source_point,
	const AxisAlignedBounds &bounds)
{
	glm::vec3 clamped = glm::clamp(source_point, bounds.min, bounds.max);
	const glm::vec3 separation = source_point - clamped;
	if (glm::length(separation) > kDirectionTolerance) {
		return NearestSurfacePoint{clamped, glm::normalize(separation)};
	}

	struct FaceDistance
	{
		float distance;
		int axis;
		float coordinate;
		float normal_sign;
	};
	const std::array<FaceDistance, 6> faces = {{
		{source_point.x - bounds.min.x, 0, bounds.min.x, -1.0f},
		{bounds.max.x - source_point.x, 0, bounds.max.x, 1.0f},
		{source_point.y - bounds.min.y, 1, bounds.min.y, -1.0f},
		{bounds.max.y - source_point.y, 1, bounds.max.y, 1.0f},
		{source_point.z - bounds.min.z, 2, bounds.min.z, -1.0f},
		{bounds.max.z - source_point.z, 2, bounds.max.z, 1.0f},
	}};
	const FaceDistance *nearest = &faces.front();
	for (const FaceDistance &face : faces) {
		if (face.distance < nearest->distance) nearest = &face;
	}
	glm::vec3 surface_point = source_point;
	surface_point[nearest->axis] = nearest->coordinate;
	return NearestSurfacePoint{
		surface_point,
		axis_normal(nearest->axis, nearest->normal_sign)};
}

bool valid_attachment_mode(SurfaceAttachmentMode mode)
{
	switch (mode) {
	case SurfaceAttachmentMode::Contact:
	case SurfaceAttachmentMode::Offset:
	case SurfaceAttachmentMode::Twine:
		return true;
	}
	return false;
}

float effective_attachment_distance(
	const SurfaceAttachmentSpecification &specification)
{
	return specification.mode() == SurfaceAttachmentMode::Contact
		? 0.0f
		: specification.attachmentDistance();
}

} // namespace

bool SurfaceAttachmentService::validate(
	const SurfaceAttachmentSpecification &specification,
	std::string *diagnostic) const
{
	auto fail = [&](const std::string &message) {
		if (diagnostic != nullptr) *diagnostic = message;
		return false;
	};
	if (diagnostic != nullptr) diagnostic->clear();
	if (specification.target().identifier().empty()) {
		return fail("Surface attachment requires a non-empty target identifier.");
	}
	if (specification.target().geometryKind() !=
	    VegetationSurfaceGeometryKind::AxisAlignedBox) {
		return fail(
			"Surface attachment supports AxisAlignedBox targets in V1C; "
			"TriangleMesh targets fail closed.");
	}
	if (!finite_bounds(specification.target().bounds())) {
		return fail("Surface attachment target bounds are invalid or non-finite.");
	}
	if (!std::isfinite(specification.attachmentDistance()) ||
	    specification.attachmentDistance() < 0.0f) {
		return fail(
			"Surface attachment distance must be finite and non-negative.");
	}
	if (!std::isfinite(specification.tolerance()) ||
	    specification.tolerance() <= 0.0f) {
		return fail("Surface attachment tolerance must be finite and positive.");
	}
	if (!valid_attachment_mode(specification.mode())) {
		return fail("Surface attachment mode is unsupported.");
	}
	return true;
}

SurfaceAttachmentResolution SurfaceAttachmentService::attachNearest(
	glm::vec3 source_point,
	const SurfaceAttachmentSpecification &specification) const
{
	std::string diagnostic;
	if (!validate(specification, &diagnostic)) {
		return SurfaceAttachmentResolution::failed(std::move(diagnostic));
	}
	if (!finite(source_point)) {
		return SurfaceAttachmentResolution::failed(
			"Surface attachment source point must be finite.");
	}
	const NearestSurfacePoint nearest = nearest_surface_point(
		source_point, specification.target().bounds());
	const float attachment_distance =
		effective_attachment_distance(specification);
	return SurfaceAttachmentResolution::succeeded(
		nearest.point,
		nearest.point + nearest.normal * attachment_distance,
		nearest.normal,
		glm::distance(source_point, nearest.point));
}

SurfaceAttachmentResolution SurfaceAttachmentService::firstContact(
	glm::vec3 segment_start,
	glm::vec3 segment_end,
	const SurfaceAttachmentSpecification &specification) const
{
	std::string diagnostic;
	if (!validate(specification, &diagnostic)) {
		return SurfaceAttachmentResolution::failed(std::move(diagnostic));
	}
	if (!finite(segment_start) || !finite(segment_end)) {
		return SurfaceAttachmentResolution::failed(
			"Surface attachment segment endpoints must be finite.");
	}
	const glm::vec3 direction = segment_end - segment_start;
	const float segment_length = glm::length(direction);
	if (segment_length <= kDirectionTolerance) {
		return SurfaceAttachmentResolution::failed(
			"Surface attachment contact requires a non-zero segment.");
	}
	const AxisAlignedBounds &bounds = specification.target().bounds();
	if (point_inside_bounds(segment_start, bounds)) {
		return SurfaceAttachmentResolution::failed(
			"Surface attachment first-contact cannot begin inside the target.");
	}

	float entry_parameter = 0.0f;
	float exit_parameter = 1.0f;
	glm::vec3 entry_normal(0.0f);
	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(direction[axis]) <= kDirectionTolerance) {
			if (segment_start[axis] < bounds.min[axis] ||
			    segment_start[axis] > bounds.max[axis]) {
				const SurfaceAttachmentResolution endpoint_resolution =
					attachNearest(segment_end, specification);
				if (endpoint_resolution.succeeded() &&
				    endpoint_resolution.sourceDistance() <=
					    specification.tolerance()) {
					return endpoint_resolution;
				}
				return SurfaceAttachmentResolution::failed(
					"Surface attachment segment does not contact the target.");
			}
			continue;
		}

		float first_parameter =
			(bounds.min[axis] - segment_start[axis]) / direction[axis];
		float second_parameter =
			(bounds.max[axis] - segment_start[axis]) / direction[axis];
		glm::vec3 first_normal = axis_normal(axis, -1.0f);
		glm::vec3 second_normal = axis_normal(axis, 1.0f);
		if (first_parameter > second_parameter) {
			std::swap(first_parameter, second_parameter);
			std::swap(first_normal, second_normal);
		}
		if (first_parameter > entry_parameter) {
			entry_parameter = first_parameter;
			entry_normal = first_normal;
		}
		exit_parameter = std::min(exit_parameter, second_parameter);
		if (entry_parameter > exit_parameter) {
			return SurfaceAttachmentResolution::failed(
				"Surface attachment segment does not contact the target.");
		}
	}

	if (entry_parameter < 0.0f || entry_parameter > 1.0f ||
	    glm::length(entry_normal) <= kDirectionTolerance) {
		const SurfaceAttachmentResolution endpoint_resolution =
			attachNearest(segment_end, specification);
		if (endpoint_resolution.succeeded() &&
		    endpoint_resolution.sourceDistance() <= specification.tolerance()) {
			return endpoint_resolution;
		}
		return SurfaceAttachmentResolution::failed(
			"Surface attachment segment does not contact the target.");
	}

	const glm::vec3 surface_point =
		segment_start + direction * entry_parameter;
	const float attachment_distance =
		effective_attachment_distance(specification);
	return SurfaceAttachmentResolution::succeeded(
		surface_point,
		surface_point + entry_normal * attachment_distance,
		entry_normal,
		segment_length * entry_parameter);
}
