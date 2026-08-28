#include "geometry/service/TaperedSweepSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

} // namespace

std::shared_ptr<const TaperedSweepShapeSpecification>
TaperedSweepSpecificationValidator::validate(
	TaperedSweepShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.path_points.size() < 2u ||
	    candidate.path_points.size() > complexity_limits_.maximumPathPoints()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"TaperedSweep requires between 2 and the configured path-point limit.";
		}
		return {};
	}
	if (candidate.radii.size() != candidate.path_points.size()) {
		if (diagnostic != nullptr) {
			*diagnostic = "TaperedSweep requires one radius for every path point.";
		}
		return {};
	}
	if (!finite(candidate.up_hint) || glm::length(candidate.up_hint) <= 1.0e-6f) {
		if (diagnostic != nullptr) {
			*diagnostic = "TaperedSweep up hint must be a finite non-zero vector.";
		}
		return {};
	}
	if (candidate.circumferential_segments < 3 ||
	    static_cast<std::size_t>(candidate.circumferential_segments) >
		    complexity_limits_.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"TaperedSweep segments must be between 3 and the profile point limit.";
		}
		return {};
	}
	for (std::size_t index = 0; index < candidate.path_points.size(); ++index) {
		if (!finite(candidate.path_points[index]) ||
		    !std::isfinite(candidate.radii[index]) || candidate.radii[index] <= 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"TaperedSweep path points and radii must be finite with positive radii.";
			}
			return {};
		}
		if (index > 0u &&
		    glm::length(candidate.path_points[index] -
		                candidate.path_points[index - 1u]) <= 1.0e-6f) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"TaperedSweep consecutive path points must be spatially distinct.";
			}
			return {};
		}
	}

	const std::size_t side_triangles =
		(candidate.path_points.size() - 1u) *
		static_cast<std::size_t>(candidate.circumferential_segments) * 2u;
	const std::size_t cap_triangles =
		(candidate.cap_policy.closesFront()
			 ? static_cast<std::size_t>(candidate.circumferential_segments)
			 : 0u) +
		(candidate.cap_policy.closesBack()
			 ? static_cast<std::size_t>(candidate.circumferential_segments)
			 : 0u);
	if (side_triangles + cap_triangles >
		    complexity_limits_.maximumGeneratedTriangles() ||
	    (side_triangles + cap_triangles) * 3u >
		    complexity_limits_.maximumGeneratedVertices()) {
		if (diagnostic != nullptr) {
			*diagnostic = "TaperedSweep exceeds configured generated mesh limits.";
		}
		return {};
	}

	std::ostringstream key;
	key << "TaperedSweep:v1:segments=" << candidate.circumferential_segments
	    << ":cap=" << candidate.cap_policy.canonicalText() << ":up="
	    << float_bits(candidate.up_hint.x) << ","
	    << float_bits(candidate.up_hint.y) << ","
	    << float_bits(candidate.up_hint.z) << ":samples=";
	for (std::size_t index = 0; index < candidate.path_points.size(); ++index) {
		const glm::vec3 &point = candidate.path_points[index];
		key << float_bits(point.x) << "," << float_bits(point.y) << ","
		    << float_bits(point.z) << "," << float_bits(candidate.radii[index])
		    << ";";
	}

	std::ostringstream canonical;
	canonical << "TaperedSweep(segments(" << candidate.circumferential_segments
	          << ") samples(";
	for (std::size_t index = 0; index < candidate.path_points.size(); ++index) {
		const glm::vec3 &point = candidate.path_points[index];
		canonical << point.x << " " << point.y << " " << point.z << " "
		          << candidate.radii[index] << " ";
	}
	canonical << ") cap(" << candidate.cap_policy.canonicalText() << "))";

	return std::make_shared<const TaperedSweepShapeSpecification>(
		std::move(candidate.path_points),
		std::move(candidate.radii),
		candidate.up_hint,
		candidate.circumferential_segments,
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical.str(),
		candidate.detail_level);
}

