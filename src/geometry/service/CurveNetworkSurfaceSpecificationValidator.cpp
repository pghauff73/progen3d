#include "geometry/service/CurveNetworkSurfaceSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

std::string double_bits(double value)
{
	if (value == 0.0) value = 0.0;
	std::uint64_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(16) << std::setfill('0') << bits;
	return text.str();
}

const char *curve_type_name(Curve3DType type)
{
	switch (type) {
	case Curve3DType::Line: return "line";
	case Curve3DType::Polyline: return "polyline";
	case Curve3DType::Bezier: return "bezier";
	case Curve3DType::CatmullRom: return "catmullRom";
	}
	return "unknown";
}

void append_curve_text(std::ostringstream &text, const Curve3D &curve)
{
	text << curve_type_name(curve.type());
	for (const glm::vec3 &point : curve.controlPoints()) {
		text << " " << point.x << " " << point.y << " " << point.z;
	}
}

}

std::shared_ptr<const CurveNetworkSurfaceShapeSpecification>
CurveNetworkSurfaceSpecificationValidator::validate(
	CurveNetworkSurfaceShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.constant_u_curves.size() > complexity_limits_.maximumSurfaceUCurves() ||
	    candidate.constant_v_curves.size() > complexity_limits_.maximumSurfaceVCurves()) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurveNetworkSurface exceeds the configured curve-network limit.";
		}
		return {};
	}
	if (candidate.samples_u < 2 || candidate.samples_v < 2 ||
	    static_cast<std::size_t>(candidate.samples_u) > complexity_limits_.maximumSurfaceSamplesPerAxis() ||
	    static_cast<std::size_t>(candidate.samples_v) > complexity_limits_.maximumSurfaceSamplesPerAxis()) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurveNetworkSurface samples per axis must be within configured limits.";
		}
		return {};
	}
	CurveNetworkSurface surface(
		std::move(candidate.constant_u_curves),
		std::move(candidate.constant_v_curves),
		candidate.intersection_tolerance);
	if (!surface.isValid(diagnostic)) return {};

	std::ostringstream canonical;
	canonical << "CurveNetworkSurface(";
	for (const Curve3D &curve : surface.constantUCurves()) {
		canonical << "uCurve(";
		append_curve_text(canonical, curve);
		canonical << ") ";
	}
	for (const Curve3D &curve : surface.constantVCurves()) {
		canonical << "vCurve(";
		append_curve_text(canonical, curve);
		canonical << ") ";
	}
	canonical << "samplesU(" << candidate.samples_u << ") samplesV("
	          << candidate.samples_v << ") tolerance("
	          << candidate.intersection_tolerance
	          << ") method(interpolatingPatchGrid))";

	std::ostringstream key;
	key << "CurveNetworkSurface:v1:samples=" << candidate.samples_u << ","
	    << candidate.samples_v << ":tolerance="
	    << double_bits(candidate.intersection_tolerance) << ":curves=";
	for (const Curve3D &curve : surface.constantUCurves()) {
		key << "u:" << curve_type_name(curve.type()) << ":";
		for (const glm::vec3 &point : curve.controlPoints()) {
			key << double_bits(point.x) << "," << double_bits(point.y) << ","
			    << double_bits(point.z) << ";";
		}
	}
	for (const Curve3D &curve : surface.constantVCurves()) {
		key << "v:" << curve_type_name(curve.type()) << ":";
		for (const glm::vec3 &point : curve.controlPoints()) {
			key << double_bits(point.x) << "," << double_bits(point.y) << ","
			    << double_bits(point.z) << ";";
		}
	}

	return std::make_shared<const CurveNetworkSurfaceShapeSpecification>(
		std::move(surface),
		candidate.samples_u,
		candidate.samples_v,
		ShapeSpecificationKey(key.str()),
		canonical.str(),
		candidate.detail_level);
}
