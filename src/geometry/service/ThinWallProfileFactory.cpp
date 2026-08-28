#include "geometry/service/ThinWallProfileFactory.h"

#include "geometry/service/Profile2DValidator.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {

bool validate_dimensions(
	float width,
	float height,
	float sheet_thickness,
	float bend_radius,
	std::string *diagnostic)
{
	if (!std::isfinite(width) || !std::isfinite(height) ||
	    !std::isfinite(sheet_thickness) || !std::isfinite(bend_radius) ||
	    width <= 0.0f || height <= 0.0f || sheet_thickness <= 0.0f ||
	    sheet_thickness * 2.0f >= std::min(width, height) || bend_radius < 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Thin-wall profile dimensions must be finite and positive, thickness must fit inside the section, and bend radius must be nonnegative.";
		}
		return false;
	}
	return true;
}

std::vector<glm::vec2> rounded_rectangle_points(
	float width,
	float height,
	float requested_radius,
	bool clockwise)
{
	const float radius = std::min(
		requested_radius,
		std::max(0.0f, std::min(width, height) * 0.5f - 1.0e-6f));
	if (radius <= 1.0e-6f) {
		std::vector<glm::vec2> rectangle = {
			{-width * 0.5f, -height * 0.5f},
			{width * 0.5f, -height * 0.5f},
			{width * 0.5f, height * 0.5f},
			{-width * 0.5f, height * 0.5f}};
		if (clockwise) std::reverse(rectangle.begin(), rectangle.end());
		return rectangle;
	}

	constexpr int segments_per_corner = 2;
	const glm::vec2 centers[4] = {
		{width * 0.5f - radius, -height * 0.5f + radius},
		{width * 0.5f - radius, height * 0.5f - radius},
		{-width * 0.5f + radius, height * 0.5f - radius},
		{-width * 0.5f + radius, -height * 0.5f + radius}};
	const float start_degrees[4] = {-90.0f, 0.0f, 90.0f, 180.0f};
	std::vector<glm::vec2> points;
	points.reserve(8u);
	for (int corner_index = 0; corner_index < 4; ++corner_index) {
		for (int segment_index = 0; segment_index < segments_per_corner; ++segment_index) {
			const float angle = glm::radians(
				start_degrees[corner_index] +
				90.0f * static_cast<float>(segment_index) /
				static_cast<float>(segments_per_corner));
			points.emplace_back(
				centers[corner_index].x + radius * std::cos(angle),
				centers[corner_index].y + radius * std::sin(angle));
		}
	}
	if (clockwise) std::reverse(points.begin(), points.end());
	return points;
}

std::shared_ptr<const ThinWallProfile2D> validate_profile(
	ThinWallProfileFamily family,
	Profile2DCandidate candidate,
	float sheet_thickness,
	float bend_radius,
	std::size_t cell_count,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	auto profile = Profile2DValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!profile) return {};
	return std::make_shared<const ThinWallProfile2D>(
		family, *profile, sheet_thickness, bend_radius, cell_count);
}

}

std::shared_ptr<const ThinWallProfile2D> ThinWallProfileFactory::createClosedBox(
	float width,
	float height,
	float sheet_thickness,
	float bend_radius,
	std::string *diagnostic) const
{
	if (!validate_dimensions(
			width, height, sheet_thickness, bend_radius, diagnostic)) {
		return {};
	}
	Profile2DCandidate candidate;
	candidate.outer_loop = rounded_rectangle_points(width, height, bend_radius, false);
	candidate.inner_loops.push_back(rounded_rectangle_points(
		width - 2.0f * sheet_thickness,
		height - 2.0f * sheet_thickness,
		std::max(0.0f, bend_radius - sheet_thickness),
		true));
	return validate_profile(
		ThinWallProfileFamily::ClosedBox, std::move(candidate), sheet_thickness,
		bend_radius, 1u, complexity_limits_, diagnostic);
}

std::shared_ptr<const ThinWallProfile2D> ThinWallProfileFactory::createOpenChannel(
	float width,
	float height,
	float sheet_thickness,
	float bend_radius,
	std::string *diagnostic) const
{
	if (!validate_dimensions(
			width, height, sheet_thickness, bend_radius, diagnostic)) {
		return {};
	}
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	Profile2DCandidate candidate;
	candidate.outer_loop = {
		{-half_width, -half_height},
		{half_width, -half_height},
		{half_width, -half_height + sheet_thickness},
		{-half_width + sheet_thickness, -half_height + sheet_thickness},
		{-half_width + sheet_thickness, half_height - sheet_thickness},
		{half_width, half_height - sheet_thickness},
		{half_width, half_height},
		{-half_width, half_height}};
	return validate_profile(
		ThinWallProfileFamily::OpenChannel, std::move(candidate), sheet_thickness,
		bend_radius, 1u, complexity_limits_, diagnostic);
}

std::shared_ptr<const ThinWallProfile2D> ThinWallProfileFactory::createHatSection(
	float width,
	float height,
	float sheet_thickness,
	float bend_radius,
	float flange_width,
	std::string *diagnostic) const
{
	if (!validate_dimensions(
			width, height, sheet_thickness, bend_radius, diagnostic) ||
	    !std::isfinite(flange_width) || flange_width <= sheet_thickness ||
	    flange_width * 2.0f >= width) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Hat-section flange width must be finite, greater than thickness, and smaller than half the section width.";
		}
		return {};
	}
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	const float web_half_width = half_width - flange_width;
	Profile2DCandidate candidate;
	candidate.outer_loop = {
		{-half_width, -half_height},
		{-web_half_width, -half_height},
		{-web_half_width, half_height - sheet_thickness},
		{web_half_width, half_height - sheet_thickness},
		{web_half_width, -half_height},
		{half_width, -half_height},
		{half_width, -half_height + sheet_thickness},
		{web_half_width + sheet_thickness, -half_height + sheet_thickness},
		{web_half_width + sheet_thickness, half_height},
		{-web_half_width - sheet_thickness, half_height},
		{-web_half_width - sheet_thickness, -half_height + sheet_thickness},
		{-half_width, -half_height + sheet_thickness}};
	return validate_profile(
		ThinWallProfileFamily::HatSection, std::move(candidate), sheet_thickness,
		bend_radius, 1u, complexity_limits_, diagnostic);
}

std::shared_ptr<const ThinWallProfile2D> ThinWallProfileFactory::createMultiCellBox(
	float width,
	float height,
	float sheet_thickness,
	float bend_radius,
	std::size_t cell_count,
	std::string *diagnostic) const
{
	if (!validate_dimensions(
			width, height, sheet_thickness, bend_radius, diagnostic) ||
	    cell_count < 2u || cell_count > complexity_limits_.maximumProfileLoops() - 1u) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Multi-cell thin-wall box requires at least two cells within the configured profile-loop limit.";
		}
		return {};
	}
	const float corner_clearance = std::max(sheet_thickness, bend_radius);
	const float inner_height = height - 2.0f * corner_clearance;
	const float available_width =
		width - 2.0f * corner_clearance -
		sheet_thickness * static_cast<float>(cell_count - 1u);
	if (available_width <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Multi-cell thin-wall box does not have enough width for the requested cells and walls.";
		}
		return {};
	}
	const float cell_width = available_width / static_cast<float>(cell_count);
	Profile2DCandidate candidate;
	candidate.outer_loop = rounded_rectangle_points(width, height, bend_radius, false);
	float minimum_x = -width * 0.5f + corner_clearance;
	for (std::size_t cell_index = 0; cell_index < cell_count; ++cell_index) {
		const float maximum_x = minimum_x + cell_width;
		candidate.inner_loops.push_back({
			{minimum_x, -inner_height * 0.5f},
			{minimum_x, inner_height * 0.5f},
			{maximum_x, inner_height * 0.5f},
			{maximum_x, -inner_height * 0.5f}});
		minimum_x = maximum_x + sheet_thickness;
	}
	return validate_profile(
		ThinWallProfileFamily::MultiCellBox, std::move(candidate), sheet_thickness,
		bend_radius, cell_count, complexity_limits_, diagnostic);
}
