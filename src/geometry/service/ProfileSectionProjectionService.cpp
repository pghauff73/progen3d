#include "geometry/service/ProfileSectionProjectionService.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {

constexpr float minimum_profile_extent = 1.0e-6f;

glm::vec2 transform_point(
	const glm::vec2 &point,
	const glm::vec2 &center,
	const glm::vec2 &scale,
	float rotation_degrees)
{
	const glm::vec2 scaled(point.x * scale.x, point.y * scale.y);
	const float angle = glm::radians(rotation_degrees);
	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	return center + glm::vec2(
		cosine * scaled.x - sine * scaled.y,
		sine * scaled.x + cosine * scaled.y);
}

bool contains_point(
	const std::vector<glm::vec2> &polygon,
	const glm::vec2 &point)
{
	bool inside = false;
	for (std::size_t current = 0u, previous = polygon.size() - 1u;
	     current < polygon.size();
	     previous = current++) {
		const glm::vec2 &first = polygon[current];
		const glm::vec2 &second = polygon[previous];
		const bool crosses_scanline = (first.y > point.y) != (second.y > point.y);
		if (!crosses_scanline) continue;
		const float intersection_x =
			(second.x - first.x) * (point.y - first.y) /
				(second.y - first.y) +
			first.x;
		if (point.x < intersection_x) inside = !inside;
	}
	return inside;
}

std::vector<glm::vec2> transform_loop(
	const ProfileLoop2D &loop,
	const glm::vec2 &center,
	const glm::vec2 &scale,
	float rotation_degrees)
{
	std::vector<glm::vec2> transformed;
	transformed.reserve(loop.points().size());
	for (const glm::vec2 &point : loop.points()) {
		transformed.push_back(transform_point(
			point, center, scale, rotation_degrees));
	}
	return transformed;
}

} // namespace

BinarySilhouette ProfileSectionProjectionService::project(
	const Profile2D &profile,
	const glm::vec2 &center,
	const glm::vec2 &scale,
	float rotation_degrees,
	const ThreeViewProjectionConfiguration &configuration) const
{
	return project(
		std::vector<Profile2D>{profile}, center, scale, rotation_degrees,
		configuration);
}

BinarySilhouette ProfileSectionProjectionService::project(
	const std::vector<Profile2D> &profiles,
	const glm::vec2 &center,
	const glm::vec2 &scale,
	float rotation_degrees,
	const ThreeViewProjectionConfiguration &configuration) const
{
	struct TransformedProfile
	{
		std::vector<glm::vec2> outer_loop;
		std::vector<std::vector<glm::vec2>> inner_loops;
	};

	std::vector<TransformedProfile> transformed_profiles;
	transformed_profiles.reserve(profiles.size());
	for (const Profile2D &profile : profiles) {
		TransformedProfile transformed_profile;
		transformed_profile.outer_loop = transform_loop(
			profile.outerLoop(), center, scale, rotation_degrees);
		transformed_profile.inner_loops.reserve(profile.innerLoops().size());
		for (const ProfileLoop2D &inner_loop : profile.innerLoops()) {
			transformed_profile.inner_loops.push_back(transform_loop(
				inner_loop, center, scale, rotation_degrees));
		}
		transformed_profiles.push_back(std::move(transformed_profile));
	}

	BinarySilhouette silhouette(configuration.resolution(), configuration.resolution());
	if (transformed_profiles.empty()) return silhouette;

	glm::vec2 minimum(std::numeric_limits<float>::max());
	glm::vec2 maximum(std::numeric_limits<float>::lowest());
	for (const TransformedProfile &profile : transformed_profiles) {
		for (const glm::vec2 &point : profile.outer_loop) {
			minimum = glm::min(minimum, point);
			maximum = glm::max(maximum, point);
		}
	}
	const glm::vec2 extent = glm::max(
		maximum - minimum,
		glm::vec2(minimum_profile_extent));
	const float image_extent = static_cast<float>(configuration.resolution() - 1u);
	const float padding = std::clamp(
		configuration.paddingRatio(), 0.0f, 0.45f) * image_extent;
	const float usable_extent = std::max(1.0f, image_extent - 2.0f * padding);
	const float image_scale = std::min(
		usable_extent / extent.x,
		usable_extent / extent.y);
	const glm::vec2 projected_size = extent * image_scale;
	const glm::vec2 image_offset(
		(image_extent - projected_size.x) * 0.5f - minimum.x * image_scale,
		(image_extent - projected_size.y) * 0.5f + maximum.y * image_scale);

	for (std::size_t image_y = 0u; image_y < silhouette.height(); ++image_y) {
		for (std::size_t image_x = 0u; image_x < silhouette.width(); ++image_x) {
			const glm::vec2 profile_point(
				(static_cast<float>(image_x) + 0.5f - image_offset.x) / image_scale,
				(image_offset.y - static_cast<float>(image_y) - 0.5f) / image_scale);
			for (const TransformedProfile &profile : transformed_profiles) {
				if (!contains_point(profile.outer_loop, profile_point)) continue;
				bool inside_void = false;
				for (const std::vector<glm::vec2> &inner_loop : profile.inner_loops) {
					if (contains_point(inner_loop, profile_point)) {
						inside_void = true;
						break;
					}
				}
				if (!inside_void) {
					silhouette.occupy(image_x, image_y);
					break;
				}
			}
		}
	}
	return silhouette;
}
