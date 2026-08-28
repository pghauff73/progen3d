#include "vehicle/service/AutomotiveClosureGeometryService.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

AxisAlignedBounds bounds_from_points(const std::vector<glm::vec3> &points)
{
	AxisAlignedBounds bounds;
	if (points.empty()) return bounds;
	bounds.min = points.front();
	bounds.max = points.front();
	for (const glm::vec3 &point : points) {
		bounds.min = glm::min(bounds.min, point);
		bounds.max = glm::max(bounds.max, point);
	}
	bounds.center = (bounds.min + bounds.max) * 0.5f;
	bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
	bounds.valid = true;
	return bounds;
}

std::array<glm::vec3, 8> corners(const AxisAlignedBounds &bounds)
{
	return {{
		{bounds.min.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.min.y, bounds.min.z},
		{bounds.min.x, bounds.max.y, bounds.min.z},
		{bounds.max.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.min.y, bounds.max.z},
		{bounds.min.x, bounds.max.y, bounds.max.z},
		{bounds.max.x, bounds.max.y, bounds.max.z}}};
}

AxisAlignedBounds transform_bounds(
	const AxisAlignedBounds &bounds,
	const glm::mat4 &transform)
{
	if (!bounds.valid) return {};
	std::vector<glm::vec3> transformed;
	transformed.reserve(8u);
	for (const glm::vec3 &corner : corners(bounds)) {
		transformed.push_back(glm::vec3(transform * glm::vec4(corner, 1.0f)));
	}
	return bounds_from_points(transformed);
}

AxisAlignedBounds merge_bounds(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second)
{
	if (!first.valid) return second;
	if (!second.valid) return first;
	AxisAlignedBounds merged;
	merged.min = glm::min(first.min, second.min);
	merged.max = glm::max(first.max, second.max);
	merged.center = (merged.min + merged.max) * 0.5f;
	merged.half_extents = (merged.max - merged.min) * 0.5f;
	merged.valid = true;
	return merged;
}

bool contains_bounds(
	const AxisAlignedBounds &container,
	const AxisAlignedBounds &content,
	float tolerance)
{
	return container.valid && content.valid &&
	       content.min.x >= container.min.x - tolerance &&
	       content.min.y >= container.min.y - tolerance &&
	       content.min.z >= container.min.z - tolerance &&
	       content.max.x <= container.max.x + tolerance &&
	       content.max.y <= container.max.y + tolerance &&
	       content.max.z <= container.max.z + tolerance;
}

} // namespace

glm::mat4 AutomotiveClosureGeometryService::calculateClosureTransform(
	const ClosureHingeStudy &hinge_study,
	float normalized_state) const
{
	const float state = glm::clamp(normalized_state, 0.0f, 1.0f);
	const float degrees = hinge_study.minimumOpeningDegrees() +
		(hinge_study.maximumOpeningDegrees() - hinge_study.minimumOpeningDegrees()) * state;
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), hinge_study.lowerHinge());
	transform = glm::rotate(transform, glm::radians(degrees), hinge_study.axis());
	return glm::translate(transform, -hinge_study.lowerHinge());
}

float AutomotiveClosureGeometryService::calculateClosureRise(
	const ClosureHingeStudy &hinge_study,
	float normalized_state) const
{
	if (!hinge_study.sourceBounds().valid) return 0.0f;
	const glm::vec3 transformed_center = glm::vec3(
		calculateClosureTransform(hinge_study, normalized_state) *
		glm::vec4(hinge_study.sourceBounds().center, 1.0f));
	return transformed_center.y - hinge_study.sourceBounds().center.y;
}

AxisAlignedBounds AutomotiveClosureGeometryService::calculateClosureSweptBounds(
	const ClosureHingeStudy &hinge_study,
	std::size_t sample_count) const
{
	AxisAlignedBounds swept;
	const std::size_t count = std::max<std::size_t>(sample_count, 2u);
	for (std::size_t index = 0u; index < count; ++index) {
		const float state = static_cast<float>(index) / static_cast<float>(count - 1u);
		swept = merge_bounds(swept, transform_bounds(
			hinge_study.sourceBounds(), calculateClosureTransform(hinge_study, state)));
	}
	return swept;
}

glm::mat4 AutomotiveClosureGeometryService::calculateHelicalGlassTransform(
	const HelicalGlassDrop &glass_drop,
	float normalized_state) const
{
	const float state = glm::clamp(normalized_state, 0.0f, 1.0f);
	const float limited_state = glass_drop.upperLimit() +
		(glass_drop.lowerLimit() - glass_drop.upperLimit()) * state;
	const glm::vec3 axis = glm::length(glass_drop.helixAxis()) > 1.0e-7f
		? glm::normalize(glass_drop.helixAxis())
		: glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 origin = glass_drop.barrelSurface().axisOrigin();
	glm::mat4 rotation = glm::translate(glm::mat4(1.0f), origin);
	rotation = glm::rotate(
		rotation,
		glm::radians(glass_drop.rotationRateDegrees() * limited_state), axis);
	rotation = glm::translate(rotation, -origin);
	return glm::translate(
		glm::mat4(1.0f), axis * glass_drop.pitch() * limited_state) * rotation;
}

AxisAlignedBounds AutomotiveClosureGeometryService::calculateHelicalGlassSweptBounds(
	const HelicalGlassDrop &glass_drop,
	std::size_t sample_count) const
{
	AxisAlignedBounds swept;
	const std::size_t count = std::max<std::size_t>(sample_count, 2u);
	for (std::size_t index = 0u; index < count; ++index) {
		const float state = static_cast<float>(index) / static_cast<float>(count - 1u);
		swept = merge_bounds(swept, transform_bounds(
			glass_drop.glassSurface().localBounds(),
			calculateHelicalGlassTransform(glass_drop, state)));
	}
	return swept;
}

bool AutomotiveClosureGeometryService::glassRemainsInsideDoorCavity(
	const HelicalGlassDrop &glass_drop,
	std::size_t sample_count,
	float tolerance) const
{
	const std::size_t count = std::max<std::size_t>(sample_count, 2u);
	for (std::size_t index = 0u; index < count; ++index) {
		const float state = static_cast<float>(index) / static_cast<float>(count - 1u);
		const AxisAlignedBounds transformed = transform_bounds(
			glass_drop.glassSurface().localBounds(),
			calculateHelicalGlassTransform(glass_drop, state));
		if (!contains_bounds(glass_drop.doorCavity(), transformed, tolerance)) {
			return false;
		}
	}
	return true;
}

GlassChannel AutomotiveClosureGeometryService::generateChannel(
	const std::string &identifier,
	const SideGlassSurface &glass_surface,
	bool front,
	float channel_width,
	float channel_depth,
	float clearance) const
{
	const AxisAlignedBounds &bounds = glass_surface.localBounds();
	const float longitudinal = front ? bounds.max.z : bounds.min.z;
	const glm::vec3 bottom(bounds.center.x, bounds.min.y, longitudinal);
	const glm::vec3 top(bounds.center.x, bounds.max.y, longitudinal);
	return GlassChannel(
		identifier,
		Curve3D(Curve3DType::Line, {top, bottom}, identifier + ".CentreLine"),
		channel_width, channel_depth, clearance);
}

VariableSealSectionStation AutomotiveClosureGeometryService::interpolateSealSection(
	const VariableSealSweep &seal,
	float parameter) const
{
	const auto &stations = seal.sectionStations();
	if (stations.empty()) return VariableSealSectionStation(parameter, 0.0f, 0.0f, 0.0f);
	const float clamped = glm::clamp(parameter, 0.0f, 1.0f);
	if (clamped <= stations.front().parameter()) return stations.front();
	if (clamped >= stations.back().parameter()) return stations.back();
	for (std::size_t index = 1u; index < stations.size(); ++index) {
		if (clamped > stations[index].parameter()) continue;
		const VariableSealSectionStation &first = stations[index - 1u];
		const VariableSealSectionStation &second = stations[index];
		const float range = second.parameter() - first.parameter();
		const float local = range > 1.0e-7f
			? (clamped - first.parameter()) / range
			: 0.0f;
		return VariableSealSectionStation(
			clamped,
			glm::mix(first.width(), second.width(), local),
			glm::mix(first.height(), second.height(), local),
			glm::mix(first.compressionTarget(), second.compressionTarget(), local));
	}
	return stations.back();
}

glm::mat4 AutomotiveClosureGeometryService::composeClosureChildTransform(
	const ClosureHingeStudy &hinge_study,
	float normalized_state,
	const glm::mat4 &child_local_transform) const
{
	return calculateClosureTransform(hinge_study, normalized_state) *
	       child_local_transform;
}
