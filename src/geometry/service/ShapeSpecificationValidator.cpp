#include "geometry/service/ShapeSpecificationValidator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {

constexpr std::size_t kMaximumGeneratedVertexBudget = 1000000;
constexpr std::size_t kMaximumGeneratedTriangleBudget = 2000000;

float canonical_float(float value)
{
	return value == 0.0f ? 0.0f : value;
}

std::string float_bits(float value)
{
	value = canonical_float(value);
	static_assert(sizeof(float) == sizeof(std::uint32_t),
	              "Shape keys require 32-bit float storage.");
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

std::string topology_name(ShapeTopology topology)
{
	switch (topology) {
	case ShapeTopology::Surface: return "surface";
	case ShapeTopology::Shell: return "shell";
	case ShapeTopology::Solid: return "solid";
	}
	return "solid";
}

std::string mapping_name(ShapeMappingMode mapping)
{
	switch (mapping) {
	case ShapeMappingMode::Cylindrical: return "cylindrical";
	case ShapeMappingMode::Spherical: return "spherical";
	case ShapeMappingMode::Triplanar: return "triplanar";
	}
	return "triplanar";
}

std::string clip_key(const PlaneClipSpecification &clip)
{
	std::ostringstream text;
	text << float_bits(clip.normal().x) << ":"
	     << float_bits(clip.normal().y) << ":"
	     << float_bits(clip.normal().z) << ":"
	     << float_bits(clip.normalizedOffset()) << ":"
	     << (clip.retainedSide() == ClipRetainedSide::Positive ? "p" : "n")
	     << ":" << static_cast<int>(clip.semantic());
	return text.str();
}

bool validate_radial_candidate(float minimum,
	                           float maximum,
	                           ShapeTopology topology,
	                           bool topology_was_explicit,
	                           ShapeTopology *resolved_topology,
	                           std::string *diagnostic)
{
	if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
	    minimum < 0.0f || maximum > 1.0f || minimum >= maximum) {
		if (diagnostic != nullptr) {
			*diagnostic = "Radial domain requires finite values with 0 <= inner < outer <= 1.";
		}
		return false;
	}

	ShapeTopology inferred = topology;
	if (!topology_was_explicit && minimum > 0.0f) {
		inferred = ShapeTopology::Shell;
	}
	if (inferred == ShapeTopology::Solid && minimum > 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Solid topology requires a zero inner radius.";
		}
		return false;
	}
	if (inferred == ShapeTopology::Shell && minimum <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Shell topology requires a positive inner radius.";
		}
		return false;
	}
	if (inferred == ShapeTopology::Surface && minimum > 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Surface topology supports only the outer curved skin.";
		}
		return false;
	}
	*resolved_topology = inferred;
	return true;
}

bool validate_clips(std::vector<PlaneClipSpecification> *clips,
	                std::string *diagnostic)
{
	for (std::size_t clip_index = 0; clip_index < clips->size(); ++clip_index) {
		const PlaneClipSpecification &clip = (*clips)[clip_index];
		const float length = glm::length(clip.normal());
		if (!std::isfinite(length) || length <= 1.0e-6f) {
			if (diagnostic != nullptr) {
				*diagnostic = "Clip normal must be finite and nonzero.";
			}
			return false;
		}
		if (!std::isfinite(clip.normalizedOffset()) ||
		    clip.normalizedOffset() < -1.0f ||
		    clip.normalizedOffset() > 1.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "Clip offset must be finite and within [-1, 1].";
			}
			return false;
		}
		(*clips)[clip_index] = PlaneClipSpecification(
			clip.normal() / length,
			canonical_float(clip.normalizedOffset()),
			clip.retainedSide(),
			clip.semantic());
	}
	return true;
}

float wrap_degrees(float degrees)
{
	float wrapped = std::fmod(degrees, 360.0f);
	if (wrapped < 0.0f) wrapped += 360.0f;
	return canonical_float(wrapped);
}

bool validate_segments(int first,
	                   int second,
	                   int second_minimum,
	                   const char *second_name,
	                   std::string *diagnostic)
{
	if (first < 3 || first > 512) {
		if (diagnostic != nullptr) {
			*diagnostic = "Circumferential or azimuth segments must be an integer in [3, 512].";
		}
		return false;
	}
	if (second < second_minimum || second > 256) {
		if (diagnostic != nullptr) {
			*diagnostic = std::string(second_name) +
			              " segments must be an integer in [" +
			              std::to_string(second_minimum) + ", 256].";
		}
		return false;
	}

	const std::size_t estimated_vertices =
		static_cast<std::size_t>(first + 1) *
		static_cast<std::size_t>(second + 1) * 8u + 4096u;
	const std::size_t estimated_triangles = estimated_vertices * 2u;
	if (estimated_vertices > kMaximumGeneratedVertexBudget ||
	    estimated_triangles > kMaximumGeneratedTriangleBudget) {
		if (diagnostic != nullptr) {
			*diagnostic = "Requested tessellation exceeds the per-shape geometry budget.";
		}
		return false;
	}
	return true;
}

std::string canonical_clips_text(
	const std::vector<PlaneClipSpecification> &clips)
{
	std::ostringstream text;
	for (const PlaneClipSpecification &clip : clips) {
		text << " clip(" << clip.normal().x << " " << clip.normal().y
		     << " " << clip.normal().z << " " << clip.normalizedOffset()
		     << " "
		     << (clip.retainedSide() == ClipRetainedSide::Positive
		         ? "positive" : "negative")
		     << ")";
	}
	return text.str();
}

}

std::shared_ptr<const CylinderShapeSpecification>
ShapeSpecificationValidator::validateCylinder(
	CylinderShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	ShapeTopology topology = candidate.topology;
	if (!validate_radial_candidate(candidate.radial_minimum,
	                               candidate.radial_maximum,
	                               candidate.topology,
	                               candidate.topology_was_explicit,
	                               &topology,
	                               diagnostic)) {
		return {};
	}
	if (!std::isfinite(candidate.axial_minimum) ||
	    !std::isfinite(candidate.axial_maximum) ||
	    candidate.axial_minimum < 0.0f || candidate.axial_maximum > 1.0f ||
	    candidate.axial_minimum >= candidate.axial_maximum) {
		if (diagnostic != nullptr) {
			*diagnostic = "Cylinder axial domain requires finite values with 0 <= min < max <= 1.";
		}
		return {};
	}
	if (!std::isfinite(candidate.azimuth_start_degrees) ||
	    !std::isfinite(candidate.azimuth_sweep_degrees) ||
	    candidate.azimuth_sweep_degrees <= 0.0f ||
	    candidate.azimuth_sweep_degrees > 360.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Cylinder azimuth sweep must be finite and within (0, 360].";
		}
		return {};
	}
	if (!validate_segments(candidate.circumferential_segments,
	                       candidate.axial_segments,
	                       1,
	                       "Axial",
	                       diagnostic) ||
	    !validate_clips(&candidate.clips, diagnostic)) {
		return {};
	}

	candidate.radial_minimum = canonical_float(candidate.radial_minimum);
	candidate.radial_maximum = canonical_float(candidate.radial_maximum);
	candidate.axial_minimum = canonical_float(candidate.axial_minimum);
	candidate.axial_maximum = canonical_float(candidate.axial_maximum);
	candidate.azimuth_start_degrees = wrap_degrees(candidate.azimuth_start_degrees);
	candidate.azimuth_sweep_degrees = canonical_float(candidate.azimuth_sweep_degrees);

	std::ostringstream key;
	key << "cyl|" << float_bits(candidate.radial_minimum) << "|"
	    << float_bits(candidate.radial_maximum) << "|"
	    << float_bits(candidate.axial_minimum) << "|"
	    << float_bits(candidate.axial_maximum) << "|"
	    << float_bits(candidate.azimuth_start_degrees) << "|"
	    << float_bits(candidate.azimuth_sweep_degrees) << "|"
	    << static_cast<int>(topology) << "|"
	    << candidate.closure_policy.canonicalText() << "|"
	    << candidate.circumferential_segments << "|"
	    << candidate.axial_segments << "|"
	    << static_cast<int>(candidate.mapping_mode);
	for (const PlaneClipSpecification &clip : candidate.clips) {
		key << "|" << clip_key(clip);
	}

	std::ostringstream text;
	text << "Cylinder(radial(" << candidate.radial_minimum << " "
	     << candidate.radial_maximum << ") axial("
	     << candidate.axial_minimum << " " << candidate.axial_maximum
	     << ") azimuth(" << candidate.azimuth_start_degrees << " "
	     << candidate.azimuth_sweep_degrees << ") topology("
	     << topology_name(topology) << ") close("
	     << candidate.closure_policy.canonicalText() << ") segments("
	     << candidate.circumferential_segments << " "
	     << candidate.axial_segments << ") mapping("
	     << mapping_name(candidate.mapping_mode) << ")"
	     << canonical_clips_text(candidate.clips) << ")";

	const bool is_default =
		topology == ShapeTopology::Solid &&
		candidate.radial_minimum == 0.0f && candidate.radial_maximum == 1.0f &&
		candidate.axial_minimum == 0.0f && candidate.axial_maximum == 1.0f &&
		candidate.azimuth_start_degrees == 0.0f &&
		candidate.azimuth_sweep_degrees == 360.0f &&
		candidate.closure_policy.canonicalText() == "all" &&
		candidate.circumferential_segments == 40 &&
		candidate.axial_segments == 1 && candidate.clips.empty();

	return std::make_shared<const CylinderShapeSpecification>(
		RadialDomainSpecification(candidate.radial_minimum,
		                          candidate.radial_maximum),
		AxialDomainSpecification(candidate.axial_minimum,
		                         candidate.axial_maximum),
		AngularDomainSpecification(candidate.azimuth_start_degrees,
		                           candidate.azimuth_sweep_degrees),
		topology,
		candidate.closure_policy,
		ShapeTessellation(candidate.circumferential_segments,
		                  candidate.axial_segments),
		candidate.mapping_mode,
		std::move(candidate.clips),
		ShapeSpecificationKey(key.str()),
		text.str(),
		is_default);
}

std::shared_ptr<const SphereShapeSpecification>
ShapeSpecificationValidator::validateSphere(
	SphereShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	ShapeTopology topology = candidate.topology;
	if (!validate_radial_candidate(candidate.radial_minimum,
	                               candidate.radial_maximum,
	                               candidate.topology,
	                               candidate.topology_was_explicit,
	                               &topology,
	                               diagnostic)) {
		return {};
	}
	if (!std::isfinite(candidate.polar_minimum_degrees) ||
	    !std::isfinite(candidate.polar_maximum_degrees) ||
	    candidate.polar_minimum_degrees < 0.0f ||
	    candidate.polar_maximum_degrees > 180.0f ||
	    candidate.polar_minimum_degrees >= candidate.polar_maximum_degrees) {
		if (diagnostic != nullptr) {
			*diagnostic = "Sphere polar domain requires finite values with 0 <= min < max <= 180.";
		}
		return {};
	}
	if (!std::isfinite(candidate.azimuth_start_degrees) ||
	    !std::isfinite(candidate.azimuth_sweep_degrees) ||
	    candidate.azimuth_sweep_degrees <= 0.0f ||
	    candidate.azimuth_sweep_degrees > 360.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Sphere azimuth sweep must be finite and within (0, 360].";
		}
		return {};
	}
	if (!validate_segments(candidate.azimuth_segments,
	                       candidate.polar_segments,
	                       2,
	                       "Polar",
	                       diagnostic) ||
	    !validate_clips(&candidate.clips, diagnostic)) {
		return {};
	}

	candidate.radial_minimum = canonical_float(candidate.radial_minimum);
	candidate.radial_maximum = canonical_float(candidate.radial_maximum);
	candidate.polar_minimum_degrees = canonical_float(candidate.polar_minimum_degrees);
	candidate.polar_maximum_degrees = canonical_float(candidate.polar_maximum_degrees);
	candidate.azimuth_start_degrees = wrap_degrees(candidate.azimuth_start_degrees);
	candidate.azimuth_sweep_degrees = canonical_float(candidate.azimuth_sweep_degrees);

	std::ostringstream key;
	key << "sph|" << float_bits(candidate.radial_minimum) << "|"
	    << float_bits(candidate.radial_maximum) << "|"
	    << float_bits(candidate.polar_minimum_degrees) << "|"
	    << float_bits(candidate.polar_maximum_degrees) << "|"
	    << float_bits(candidate.azimuth_start_degrees) << "|"
	    << float_bits(candidate.azimuth_sweep_degrees) << "|"
	    << static_cast<int>(topology) << "|"
	    << candidate.closure_policy.canonicalText() << "|"
	    << candidate.azimuth_segments << "|"
	    << candidate.polar_segments << "|"
	    << static_cast<int>(candidate.mapping_mode);
	for (const PlaneClipSpecification &clip : candidate.clips) {
		key << "|" << clip_key(clip);
	}

	std::ostringstream text;
	text << "Sphere(radial(" << candidate.radial_minimum << " "
	     << candidate.radial_maximum << ") polar("
	     << candidate.polar_minimum_degrees << " "
	     << candidate.polar_maximum_degrees << ") azimuth("
	     << candidate.azimuth_start_degrees << " "
	     << candidate.azimuth_sweep_degrees << ") topology("
	     << topology_name(topology) << ") close("
	     << candidate.closure_policy.canonicalText() << ") segments("
	     << candidate.azimuth_segments << " "
	     << candidate.polar_segments << ") mapping("
	     << mapping_name(candidate.mapping_mode) << ")"
	     << canonical_clips_text(candidate.clips) << ")";

	const bool is_default =
		topology == ShapeTopology::Solid &&
		candidate.radial_minimum == 0.0f && candidate.radial_maximum == 1.0f &&
		candidate.polar_minimum_degrees == 0.0f &&
		candidate.polar_maximum_degrees == 180.0f &&
		candidate.azimuth_start_degrees == 0.0f &&
		candidate.azimuth_sweep_degrees == 360.0f &&
		candidate.closure_policy.canonicalText() == "all" &&
		candidate.azimuth_segments == 40 &&
		candidate.polar_segments == 20 && candidate.clips.empty();

	return std::make_shared<const SphereShapeSpecification>(
		RadialDomainSpecification(candidate.radial_minimum,
		                          candidate.radial_maximum),
		PolarDomainSpecification(candidate.polar_minimum_degrees,
		                         candidate.polar_maximum_degrees),
		AngularDomainSpecification(candidate.azimuth_start_degrees,
		                           candidate.azimuth_sweep_degrees),
		topology,
		candidate.closure_policy,
		ShapeTessellation(candidate.azimuth_segments,
		                  candidate.polar_segments),
		candidate.mapping_mode,
		std::move(candidate.clips),
		ShapeSpecificationKey(key.str()),
		text.str(),
		is_default);
}
