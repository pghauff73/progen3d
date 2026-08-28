#include "geometry/service/Profile2DFactory.h"

#include "geometry/service/Profile2DValidator.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace {

bool validate_positive_dimension(float value,
	                             const std::string &name,
	                             std::string *diagnostic)
{
	if (std::isfinite(value) && value > 0.0f) return true;
	if (diagnostic != nullptr) {
		*diagnostic = name + " must be finite and greater than zero.";
	}
	return false;
}

}

std::shared_ptr<const Profile2D> Profile2DFactory::createRectangle(
	float width,
	float height,
	std::string *diagnostic) const
{
	if (!validate_positive_dimension(width, "Rectangle width", diagnostic) ||
	    !validate_positive_dimension(height, "Rectangle height", diagnostic)) {
		return {};
	}
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	return createPolygon(
		{{-half_width, -half_height},
		 {half_width, -half_height},
		 {half_width, half_height},
		 {-half_width, half_height}},
		{},
		diagnostic);
}

std::shared_ptr<const Profile2D> Profile2DFactory::createRoundedRectangle(
	float width,
	float height,
	float corner_radius,
	int segments_per_corner,
	std::string *diagnostic) const
{
	if (!validate_positive_dimension(width, "Rounded rectangle width", diagnostic) ||
	    !validate_positive_dimension(height, "Rounded rectangle height", diagnostic) ||
	    !std::isfinite(corner_radius) || corner_radius <= 0.0f ||
	    corner_radius * 2.0f >= std::min(width, height)) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Rounded rectangle radius must be finite, positive, and smaller than half the minimum dimension.";
		}
		return {};
	}
	if (segments_per_corner < 1) {
		if (diagnostic != nullptr) {
			*diagnostic = "Rounded rectangle requires at least one segment per corner.";
		}
		return {};
	}

	const glm::vec2 centers[4] = {
		{width * 0.5f - corner_radius, -height * 0.5f + corner_radius},
		{width * 0.5f - corner_radius, height * 0.5f - corner_radius},
		{-width * 0.5f + corner_radius, height * 0.5f - corner_radius},
		{-width * 0.5f + corner_radius, -height * 0.5f + corner_radius}};
	const float start_degrees[4] = {-90.0f, 0.0f, 90.0f, 180.0f};
	std::vector<glm::vec2> points;
	points.reserve(static_cast<std::size_t>(segments_per_corner + 1) * 4u);
	for (int corner_index = 0; corner_index < 4; ++corner_index) {
		for (int segment_index = 0;
		     segment_index <= segments_per_corner;
		     ++segment_index) {
			const float angle = glm::radians(
				start_degrees[corner_index] +
				90.0f * static_cast<float>(segment_index) /
				static_cast<float>(segments_per_corner));
			points.emplace_back(
				centers[corner_index].x + corner_radius * std::cos(angle),
				centers[corner_index].y + corner_radius * std::sin(angle));
		}
	}
	return createPolygon(std::move(points), {}, diagnostic);
}

std::shared_ptr<const Profile2D> Profile2DFactory::createCircle(
	float radius,
	int segments,
	std::string *diagnostic) const
{
	return createEllipse(radius, radius, segments, diagnostic);
}

std::shared_ptr<const Profile2D> Profile2DFactory::createEllipse(
	float radius_x,
	float radius_y,
	int segments,
	std::string *diagnostic) const
{
	if (!validate_positive_dimension(radius_x, "Ellipse X radius", diagnostic) ||
	    !validate_positive_dimension(radius_y, "Ellipse Y radius", diagnostic)) {
		return {};
	}
	if (segments < 3 ||
	    static_cast<std::size_t>(segments) >
		    complexity_limits_.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Ellipse segments must be between 3 and the configured profile point limit.";
		}
		return {};
	}
	std::vector<glm::vec2> points;
	points.reserve(static_cast<std::size_t>(segments));
	for (int segment_index = 0; segment_index < segments; ++segment_index) {
		const float angle = glm::two_pi<float>() *
		                    static_cast<float>(segment_index) /
		                    static_cast<float>(segments);
		points.emplace_back(radius_x * std::cos(angle), radius_y * std::sin(angle));
	}
	return createPolygon(std::move(points), {}, diagnostic);
}

std::shared_ptr<const Profile2D> Profile2DFactory::createChamferRectangle(
	float width,
	float height,
	float chamfer,
	std::string *diagnostic) const
{
	if (!validate_positive_dimension(width, "Chamfer rectangle width", diagnostic) ||
	    !validate_positive_dimension(height, "Chamfer rectangle height", diagnostic) ||
	    !std::isfinite(chamfer) || chamfer <= 0.0f ||
	    chamfer * 2.0f >= std::min(width, height)) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Chamfer must be finite, positive, and smaller than half the minimum dimension.";
		}
		return {};
	}
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	return createPolygon(
		{{-half_width + chamfer, -half_height},
		 {half_width - chamfer, -half_height},
		 {half_width, -half_height + chamfer},
		 {half_width, half_height - chamfer},
		 {half_width - chamfer, half_height},
		 {-half_width + chamfer, half_height},
		 {-half_width, half_height - chamfer},
		 {-half_width, -half_height + chamfer}},
		{},
		diagnostic);
}

std::shared_ptr<const Profile2D> Profile2DFactory::createPolygon(
	std::vector<glm::vec2> outer_loop,
	std::vector<std::vector<glm::vec2>> inner_loops,
	std::string *diagnostic) const
{
	return Profile2DValidator(complexity_limits_).validate(
		Profile2DCandidate{std::move(outer_loop), std::move(inner_loops)},
		diagnostic);
}
