#include "geometry/service/BranchJunctionSpecificationValidator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
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
	std::uint32_t bits = 0u;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << bits;
	return text.str();
}

} // namespace

std::shared_ptr<const BranchJunctionShapeSpecification>
BranchJunctionSpecificationValidator::validate(
	BranchJunctionShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.core_radius) || candidate.core_radius <= 0.0f ||
	    !std::isfinite(candidate.bulge_scale) || candidate.bulge_scale < 1.0f ||
	    candidate.bulge_scale > 4.0f) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"BranchJunction core radius must be positive and bulge scale must be in [1, 4].";
		}
		return {};
	}
	if (candidate.arms.size() < 3u ||
	    candidate.arms.size() > complexity_limits_.maximumPathPoints()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"BranchJunction requires one parent arm and at least two child arms within the path-point limit.";
		}
		return {};
	}
	if (candidate.circumferential_segments < 3 ||
	    static_cast<std::size_t>(candidate.circumferential_segments) >
		    complexity_limits_.maximumPointsPerProfile()) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"BranchJunction segments must be between 3 and the profile point limit.";
		}
		return {};
	}

	std::size_t parent_count = 0u;
	std::size_t child_count = 0u;
	std::vector<BranchJunctionArmSpecification> normalized_arms;
	normalized_arms.reserve(candidate.arms.size());
	for (const BranchJunctionArmSpecification &arm : candidate.arms) {
		if (!finite(arm.direction()) || glm::length(arm.direction()) <= 1.0e-6f ||
		    !std::isfinite(arm.radius()) || arm.radius() <= 0.0f ||
		    !std::isfinite(arm.transitionLength()) ||
		    arm.transitionLength() <= 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"BranchJunction arms require finite non-zero directions, positive radii, and positive transition lengths.";
			}
			return {};
		}
		if (arm.role() == BranchJunctionArmRole::Parent) ++parent_count;
		else ++child_count;
		normalized_arms.emplace_back(
			arm.role(), glm::normalize(arm.direction()), arm.radius(),
			arm.transitionLength());
	}
	if (parent_count != 1u || child_count < 2u) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"BranchJunction requires exactly one parent arm and at least two child arms.";
		}
		return {};
	}
	for (std::size_t first = 0u; first < normalized_arms.size(); ++first) {
		for (std::size_t second = first + 1u;
		     second < normalized_arms.size(); ++second) {
			if (glm::dot(
					normalized_arms[first].direction(),
					normalized_arms[second].direction()) > 0.9999f) {
				if (diagnostic != nullptr) {
					*diagnostic =
						"BranchJunction arm directions must be spatially distinct.";
				}
				return {};
			}
		}
	}

	const std::size_t estimated_triangles =
		static_cast<std::size_t>(candidate.circumferential_segments) *
		(static_cast<std::size_t>(candidate.circumferential_segments) * 4u +
		 normalized_arms.size() * 4u);
	if (estimated_triangles > complexity_limits_.maximumGeneratedTriangles() ||
	    estimated_triangles * 3u >
		    complexity_limits_.maximumGeneratedVertices()) {
		if (diagnostic != nullptr) {
			*diagnostic = "BranchJunction exceeds configured generated mesh limits.";
		}
		return {};
	}

	std::ostringstream key;
	key << "BranchJunction:v1:core=" << float_bits(candidate.core_radius)
	    << ":bulge=" << float_bits(candidate.bulge_scale)
	    << ":segments=" << candidate.circumferential_segments << ":arms=";
	std::ostringstream canonical;
	canonical << "BranchJunction(core(" << candidate.core_radius << " "
	          << candidate.bulge_scale << ") ";
	for (const BranchJunctionArmSpecification &arm : normalized_arms) {
		key << branchJunctionArmRoleName(arm.role()) << ","
		    << float_bits(arm.direction().x) << ","
		    << float_bits(arm.direction().y) << ","
		    << float_bits(arm.direction().z) << ","
		    << float_bits(arm.radius()) << ","
		    << float_bits(arm.transitionLength()) << ";";
		canonical << (arm.role() == BranchJunctionArmRole::Parent
			? "parent(" : "child(")
		          << arm.direction().x << " " << arm.direction().y << " "
		          << arm.direction().z << " " << arm.radius() << " "
		          << arm.transitionLength() << ") ";
	}
	canonical << "segments(" << candidate.circumferential_segments << "))";
	if (diagnostic != nullptr) diagnostic->clear();
	return std::make_shared<const BranchJunctionShapeSpecification>(
		candidate.core_radius, candidate.bulge_scale, std::move(normalized_arms),
		candidate.circumferential_segments, ShapeSpecificationKey(key.str()),
		canonical.str(), candidate.detail_level);
}
