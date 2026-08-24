#include "geometry/service/SweepDiskSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

const char *curve_type_name(Curve3DType type)
{
	switch (type) {
	case Curve3DType::Line:
		return "line";
	case Curve3DType::Polyline:
		return "polyline";
	case Curve3DType::Bezier:
		return "bezier";
	case Curve3DType::CatmullRom:
		return "catmullRom";
	}
	return "unknown";
}

}

std::shared_ptr<const SweepDiskShapeSpecification>
SweepDiskSpecificationValidator::validate(
	SweepDiskShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.up_hint.x) ||
	    !std::isfinite(candidate.up_hint.y) ||
	    !std::isfinite(candidate.up_hint.z) ||
	    glm::dot(candidate.up_hint, candidate.up_hint) <= 1.0e-12f) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk up hint must be finite and non-zero.";
		}
		return {};
	}
	if (candidate.radius_was_explicit &&
	    (candidate.radius_start_was_explicit || candidate.radius_end_was_explicit)) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk radius cannot be combined with radiusStart or radiusEnd.";
		}
		return {};
	}

	const float radius_start = candidate.radius_start_was_explicit
		? candidate.radius_start
		: candidate.radius;
	const float radius_end = candidate.radius_end_was_explicit
		? candidate.radius_end
		: candidate.radius;
	if (!std::isfinite(radius_start) || radius_start <= 0.0f ||
	    !std::isfinite(radius_end) || radius_end <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk start and end radii must be finite and greater than zero.";
		}
		return {};
	}
	if (candidate.circumferential_segments < 3 ||
	    static_cast<std::size_t>(candidate.circumferential_segments) >
		    complexity_limits_.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk radial segments must be between 3 and the configured profile point limit.";
		}
		return {};
	}

	std::vector<glm::vec3> control_points = candidate.curve_was_explicit
		? std::move(candidate.curve_control_points)
		: std::move(candidate.path_points);
	const Curve3DType curve_type = candidate.curve_was_explicit
		? candidate.curve_type
		: Curve3DType::Polyline;
	if (control_points.size() > complexity_limits_.maximumCurveControlPoints()) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk curve exceeds the configured control-point limit.";
		}
		return {};
	}
	Curve3D path_curve(curve_type, control_points, "SweepDisk.Path");
	std::string curve_diagnostic;
	if (!path_curve.isValid(&curve_diagnostic)) {
		if (diagnostic != nullptr) *diagnostic = std::move(curve_diagnostic);
		return {};
	}

	int longitudinal_segments = candidate.longitudinal_segments;
	if (longitudinal_segments == 0) {
		longitudinal_segments = candidate.curve_was_explicit
			? 24
			: static_cast<int>(control_points.size() - 1u);
	}
	if (longitudinal_segments < 1 ||
	    static_cast<std::size_t>(longitudinal_segments + 1) >
		    complexity_limits_.maximumCurveSamples()) {
		if (diagnostic != nullptr) {
			*diagnostic = "SweepDisk longitudinal segments exceed the configured curve sample limit.";
		}
		return {};
	}

	std::vector<glm::vec3> sampled_path;
	if (!candidate.curve_was_explicit &&
	    longitudinal_segments == static_cast<int>(control_points.size() - 1u)) {
		sampled_path = control_points;
	}
	else {
		const std::vector<glm::dvec3> samples = path_curve.sampleByArcLength(
			static_cast<std::size_t>(longitudinal_segments + 1),
			std::min<std::size_t>(
				complexity_limits_.maximumCurveSamples(),
				std::max<std::size_t>(257u,
					static_cast<std::size_t>(longitudinal_segments * 8 + 1))));
		if (samples.size() != static_cast<std::size_t>(longitudinal_segments + 1)) {
			if (diagnostic != nullptr) {
				*diagnostic = "SweepDisk failed to sample its curve by arc length.";
			}
			return {};
		}
		sampled_path.reserve(samples.size());
		for (const glm::dvec3 &point : samples) sampled_path.emplace_back(point);
	}

	std::ostringstream key;
	key << "SweepDisk:v2:curve=" << curve_type_name(curve_type)
	    << ":r0=" << float_bits(radius_start)
	    << ":r1=" << float_bits(radius_end)
	    << ":longitudinal=" << longitudinal_segments
	    << ":radial=" << candidate.circumferential_segments
	    << ":cap=" << candidate.cap_policy.canonicalText()
	    << ":up=" << float_bits(candidate.up_hint.x) << ","
	    << float_bits(candidate.up_hint.y) << ","
	    << float_bits(candidate.up_hint.z) << ":control=";
	for (const glm::vec3 &point : control_points) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ","
		    << float_bits(point.z) << ";";
	}

	std::ostringstream canonical_text;
	canonical_text << "SweepDisk(curve(" << curve_type_name(curve_type) << " ";
	for (const glm::vec3 &point : control_points) {
		canonical_text << point.x << " " << point.y << " " << point.z << " ";
	}
	canonical_text << ") radiusStart(" << radius_start << ") radiusEnd("
	               << radius_end << ") longitudinalSegments("
	               << longitudinal_segments << ") radialSegments("
	               << candidate.circumferential_segments << ") cap("
	               << candidate.cap_policy.canonicalText() << "))";

	return std::make_shared<const SweepDiskShapeSpecification>(
		std::move(path_curve),
		std::move(sampled_path),
		candidate.up_hint,
		radius_start,
		radius_end,
		longitudinal_segments,
		candidate.circumferential_segments,
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical_text.str(),
		candidate.detail_level);
}
