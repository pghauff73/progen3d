#include "geometry/service/BotanicalBladeSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>

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

const char *kind_name(BotanicalBladeKind kind)
{
	return kind == BotanicalBladeKind::Leaf ? "LeafBlade" : "PetalBlade";
}

} // namespace

std::shared_ptr<const BotanicalBladeShapeSpecification>
BotanicalBladeSpecificationValidator::validate(
	BotanicalBladeShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.length) || candidate.length <= 0.0f ||
	    !std::isfinite(candidate.maximum_width) || candidate.maximum_width <= 0.0f ||
	    !std::isfinite(candidate.longitudinal_curvature) ||
	    !std::isfinite(candidate.camber) ||
	    !std::isfinite(candidate.twist_degrees) ||
	    !std::isfinite(candidate.thickness) || candidate.thickness < 0.0f ||
	    !std::isfinite(candidate.width_power) || candidate.width_power <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Botanical blade dimensions, curvature, twist, thickness, and width power must be finite; dimensions and width power must be positive.";
		}
		return {};
	}
	if (candidate.longitudinal_segments < 2 || candidate.lateral_segments < 1 ||
	    static_cast<std::size_t>(candidate.longitudinal_segments + 1) >
		    complexity_limits_.maximumPathPoints() ||
	    static_cast<std::size_t>(candidate.lateral_segments + 1) >
		    complexity_limits_.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Botanical blade tessellation exceeds configured path or profile limits.";
		}
		return {};
	}
	const std::size_t surface_count = candidate.thickness > 0.0f ? 2u : 1u;
	const std::size_t surface_triangles =
		static_cast<std::size_t>(candidate.longitudinal_segments) *
		static_cast<std::size_t>(candidate.lateral_segments) * 2u * surface_count;
	const std::size_t edge_triangles = candidate.thickness > 0.0f
		? static_cast<std::size_t>(candidate.longitudinal_segments * 2 +
		                           candidate.lateral_segments * 2) * 2u
		: 0u;
	if (surface_triangles + edge_triangles >
		    complexity_limits_.maximumGeneratedTriangles() ||
	    (surface_triangles + edge_triangles) * 3u >
		    complexity_limits_.maximumGeneratedVertices()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Botanical blade exceeds configured generated mesh limits.";
		}
		return {};
	}

	std::ostringstream key;
	key << kind_name(candidate.kind) << ":v1:profile="
	    << static_cast<int>(candidate.profile) << ":length="
	    << float_bits(candidate.length) << ":width="
	    << float_bits(candidate.maximum_width) << ":curve="
	    << float_bits(candidate.longitudinal_curvature) << ":camber="
	    << float_bits(candidate.camber) << ":twist="
	    << float_bits(candidate.twist_degrees) << ":thickness="
	    << float_bits(candidate.thickness) << ":power="
	    << float_bits(candidate.width_power) << ":segments="
	    << candidate.longitudinal_segments << "," << candidate.lateral_segments;

	std::ostringstream canonical;
	canonical << kind_name(candidate.kind) << "(profile("
	          << botanicalBladeProfileName(candidate.profile) << ") length("
	          << candidate.length << ") width(" << candidate.maximum_width
	          << ") curvature(" << candidate.longitudinal_curvature
	          << ") camber(" << candidate.camber << ") twist("
	          << candidate.twist_degrees << ") thickness("
	          << candidate.thickness << ") widthPower("
	          << candidate.width_power << ") segments("
	          << candidate.longitudinal_segments << " "
	          << candidate.lateral_segments << "))";

	if (candidate.kind == BotanicalBladeKind::Leaf) {
		return std::make_shared<const LeafBladeShapeSpecification>(
			candidate.profile, candidate.length, candidate.maximum_width,
			candidate.longitudinal_curvature, candidate.camber,
			candidate.twist_degrees, candidate.thickness,
			candidate.width_power, candidate.longitudinal_segments,
			candidate.lateral_segments, ShapeSpecificationKey(key.str()),
			canonical.str(), candidate.detail_level);
	}
	return std::make_shared<const PetalBladeShapeSpecification>(
		candidate.profile, candidate.length, candidate.maximum_width,
		candidate.longitudinal_curvature, candidate.camber,
		candidate.twist_degrees, candidate.thickness,
		candidate.width_power, candidate.longitudinal_segments,
		candidate.lateral_segments, ShapeSpecificationKey(key.str()),
		canonical.str(), candidate.detail_level);
}

