#include "geometry/service/Profile2DValidator.h"

#include "geometry/service/PolygonContainmentAnalyzer.h"
#include "geometry/service/SimplePolygonValidator.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float kProfileTolerance = 1.0e-6f;

bool approximately_equal(const glm::vec2 &first, const glm::vec2 &second)
{
	const glm::vec2 difference = first - second;
	return glm::dot(difference, difference) <=
	       kProfileTolerance * kProfileTolerance;
}

bool validate_loop_size(const std::vector<glm::vec2> &loop,
	                    const GeometryComplexityLimits &limits,
	                    const std::string &loop_name,
	                    std::string *diagnostic)
{
	std::size_t normalized_size = loop.size();
	if (normalized_size > 1 && approximately_equal(loop.front(), loop.back())) {
		--normalized_size;
	}
	if (normalized_size > limits.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic = loop_name + " exceeds the configured point limit of " +
			              std::to_string(limits.maximumPointsPerProfile()) + ".";
		}
		return false;
	}
	return true;
}

bool normalize_loop(const std::vector<glm::vec2> &input,
	                bool counter_clockwise,
	                const GeometryComplexityLimits &limits,
	                const std::string &loop_name,
	                ProfileLoop2D *normalized_loop,
	                std::string *diagnostic)
{
	if (normalized_loop == nullptr ||
	    !validate_loop_size(input, limits, loop_name, diagnostic)) {
		return false;
	}
	const float input_area = SimplePolygonValidator::signedArea(input);
	std::vector<glm::vec2> normalized_points;
	std::string polygon_diagnostic;
	if (!SimplePolygonValidator().normalize(
			input, &normalized_points, &polygon_diagnostic)) {
		if (diagnostic != nullptr) {
			const std::size_t separator = polygon_diagnostic.find(' ');
			*diagnostic = loop_name +
			              (separator == std::string::npos
				               ? std::string(" is invalid.")
				               : polygon_diagnostic.substr(separator));
		}
		return false;
	}

	ProfileWindingCorrection correction = ProfileWindingCorrection::None;
	if (counter_clockwise) {
		if (input_area < 0.0f) {
			correction = ProfileWindingCorrection::ReversedToCounterClockwise;
		}
	}
	else {
		std::reverse(normalized_points.begin(), normalized_points.end());
		if (input_area > 0.0f) {
			correction = ProfileWindingCorrection::ReversedToClockwise;
		}
	}
	*normalized_loop = ProfileLoop2D(std::move(normalized_points), correction);
	return true;
}

}

std::shared_ptr<const Profile2D> Profile2DValidator::validate(
	Profile2DCandidate candidate,
	std::string *diagnostic) const
{
	if (1u + candidate.inner_loops.size() >
	    complexity_limits_.maximumProfileLoops()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile2D exceeds the configured loop limit of " +
			              std::to_string(complexity_limits_.maximumProfileLoops()) + ".";
		}
		return {};
	}

	ProfileLoop2D outer_loop({}, ProfileWindingCorrection::None);
	if (!normalize_loop(candidate.outer_loop,
	                    true,
	                    complexity_limits_,
	                    "Profile2D outer loop",
	                    &outer_loop,
	                    diagnostic)) {
		return {};
	}

	std::vector<ProfileLoop2D> inner_loops;
	inner_loops.reserve(candidate.inner_loops.size());
	for (std::size_t hole_index = 0;
	     hole_index < candidate.inner_loops.size();
	     ++hole_index) {
		ProfileLoop2D inner_loop({}, ProfileWindingCorrection::None);
		if (!normalize_loop(candidate.inner_loops[hole_index],
		                    false,
		                    complexity_limits_,
		                    "Profile2D hole " + std::to_string(hole_index),
		                    &inner_loop,
		                    diagnostic)) {
			return {};
		}
		inner_loops.push_back(std::move(inner_loop));
	}

	PolygonContainmentAnalyzer containment_analyzer;
	for (std::size_t hole_index = 0;
	     hole_index < inner_loops.size();
	     ++hole_index) {
		if (containment_analyzer.analyze(
				outer_loop.points(), inner_loops[hole_index].points()) !=
		    PolygonContainmentRelationship::FirstContainsSecond) {
			if (diagnostic != nullptr) {
				*diagnostic = "Profile2D hole " + std::to_string(hole_index) +
				              " must be strictly inside the outer loop.";
			}
			return {};
		}
		for (std::size_t other_index = 0;
		     other_index < hole_index;
		     ++other_index) {
			if (containment_analyzer.analyze(
					inner_loops[hole_index].points(),
					inner_loops[other_index].points()) !=
			    PolygonContainmentRelationship::Disjoint) {
				if (diagnostic != nullptr) {
					*diagnostic = "Profile2D holes " +
					              std::to_string(other_index) + " and " +
					              std::to_string(hole_index) +
					              " overlap, touch, or contain one another.";
				}
				return {};
			}
		}
	}

	return std::make_shared<const Profile2D>(
		std::move(outer_loop), std::move(inner_loops));
}
