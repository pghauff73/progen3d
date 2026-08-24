#include "geometry/service/SweepProfileSpecificationValidator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

constexpr float kSweepTolerance = 1.0e-6f;

float canonical_float(float value)
{
	return value == 0.0f ? 0.0f : value;
}

std::string float_bits(float value)
{
	value = canonical_float(value);
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

}

std::shared_ptr<const SweepProfileShapeSpecification>
SweepProfileSpecificationValidator::validate(
	SweepProfileShapeSpecificationCandidate candidate,
	std::string *diagnostic) const

{
	return validateAsFamily(
		std::move(candidate), ShapeFamily::SweepProfile, diagnostic);
}

std::shared_ptr<const PanelCutShapeSpecification>
SweepProfileSpecificationValidator::validatePanelCut(
	SweepProfileShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	return std::dynamic_pointer_cast<const PanelCutShapeSpecification>(
		validateAsFamily(std::move(candidate), ShapeFamily::PanelCut, diagnostic));
}

std::shared_ptr<const SweepProfileShapeSpecification>
SweepProfileSpecificationValidator::validateAsFamily(
	SweepProfileShapeSpecificationCandidate candidate,
	ShapeFamily family,
	std::string *diagnostic) const
{
	const std::string shape_name =
		family == ShapeFamily::PanelCut ? "PanelCut" : "SweepProfile";
	if (candidate.path_points.size() < 2u) {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " requires at least two path points.";
		}
		return {};
	}
	if (candidate.path_points.size() > complexity_limits_.maximumPathPoints()) {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " exceeds the configured path point limit of " +
			              std::to_string(complexity_limits_.maximumPathPoints()) + ".";
		}
		return {};
	}
	if (!finite(candidate.up_hint) ||
	    glm::dot(candidate.up_hint, candidate.up_hint) <=
		    kSweepTolerance * kSweepTolerance) {
		if (diagnostic != nullptr) {
			*diagnostic = shape_name + " up hint must be finite and non-zero.";
		}
		return {};
	}
	for (std::size_t point_index = 0;
	     point_index < candidate.path_points.size();
	     ++point_index) {
		if (!finite(candidate.path_points[point_index])) {
			if (diagnostic != nullptr) {
				*diagnostic = shape_name + " path coordinates must be finite.";
			}
			return {};
		}
		if (point_index > 0) {
			const glm::vec3 segment = candidate.path_points[point_index] -
			                          candidate.path_points[point_index - 1u];
			if (glm::dot(segment, segment) <= kSweepTolerance * kSweepTolerance) {
				if (diagnostic != nullptr) {
					*diagnostic = shape_name + " path cannot contain zero-length segments.";
				}
				return {};
			}
			const glm::vec3 tangent = glm::normalize(segment);
			if (glm::dot(glm::cross(candidate.up_hint, tangent),
			             glm::cross(candidate.up_hint, tangent)) <=
			    kSweepTolerance * kSweepTolerance) {
				if (diagnostic != nullptr) {
					*diagnostic = shape_name + " up hint cannot be parallel to a path segment.";
				}
				return {};
			}
		}
	}

	std::shared_ptr<const Profile2D> profile =
		Profile2DValidator(complexity_limits_).validate(
			std::move(candidate.profile), diagnostic);
	if (!profile) return {};

	std::ostringstream key;
	key << shape_name << ":v1:cap=" << candidate.cap_policy.canonicalText()
	    << ":up=" << float_bits(candidate.up_hint.x) << ","
	    << float_bits(candidate.up_hint.y) << ","
	    << float_bits(candidate.up_hint.z) << ":path=";
	for (const glm::vec3 &point : candidate.path_points) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ","
		    << float_bits(point.z) << ";";
	}
	key << ":outer=";
	for (const glm::vec2 &point : profile->outerLoop().points()) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
	for (const ProfileLoop2D &inner_loop : profile->innerLoops()) {
		key << ":hole=";
		for (const glm::vec2 &point : inner_loop.points()) {
			key << float_bits(point.x) << "," << float_bits(point.y) << ";";
		}
	}

	std::ostringstream canonical_text;
	canonical_text << shape_name << "(path(";
	for (std::size_t point_index = 0;
	     point_index < candidate.path_points.size();
	     ++point_index) {
		if (point_index != 0) canonical_text << " ";
		const glm::vec3 &point = candidate.path_points[point_index];
		canonical_text << point.x << " " << point.y << " " << point.z;
	}
	canonical_text << ") up(" << candidate.up_hint.x << " "
	               << candidate.up_hint.y << " " << candidate.up_hint.z
	               << ") cap(" << candidate.cap_policy.canonicalText() << "))";

	if (family == ShapeFamily::PanelCut) {
		return std::make_shared<const PanelCutShapeSpecification>(
			*profile, std::move(candidate.path_points), candidate.up_hint,
			candidate.cap_policy, ShapeSpecificationKey(key.str()),
			canonical_text.str(), candidate.detail_level);
	}
	return std::make_shared<const SweepProfileShapeSpecification>(
		*profile, std::move(candidate.path_points), candidate.up_hint,
		candidate.cap_policy, ShapeSpecificationKey(key.str()),
		canonical_text.str(), candidate.detail_level);
}
