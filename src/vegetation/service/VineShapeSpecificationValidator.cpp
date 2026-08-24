#include "vegetation/service/VineShapeSpecificationValidator.h"

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationSurfaceGeometryKind.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x && bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
}

bool valid_growth_mode(VineGrowthMode mode)
{
	switch (mode) {
	case VineGrowthMode::FreeClimbing:
	case VineGrowthMode::WallClimbing:
	case VineGrowthMode::TrellisClimbing:
	case VineGrowthMode::GroundCreeping:
	case VineGrowthMode::Hanging:
	case VineGrowthMode::Twining:
	case VineGrowthMode::TendrilClimbing:
		return true;
	}
	return false;
}

bool valid_collision_behavior(VegetationCollisionBehavior behavior)
{
	switch (behavior) {
	case VegetationCollisionBehavior::Avoid:
	case VegetationCollisionBehavior::Seek:
	case VegetationCollisionBehavior::PermitIntersection:
		return true;
	}
	return false;
}

bool valid_attachment_mode(SurfaceAttachmentMode mode)
{
	switch (mode) {
	case SurfaceAttachmentMode::Contact:
	case SurfaceAttachmentMode::Offset:
	case SurfaceAttachmentMode::Twine:
		return true;
	}
	return false;
}

bool target_required(VineGrowthMode mode)
{
	return mode == VineGrowthMode::WallClimbing ||
	       mode == VineGrowthMode::TrellisClimbing ||
	       mode == VineGrowthMode::Twining ||
	       mode == VineGrowthMode::TendrilClimbing;
}

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0u;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

void append_bounds(std::ostringstream *stream, const AxisAlignedBounds &bounds)
{
	*stream << float_bits(bounds.min.x) << ',' << float_bits(bounds.min.y) << ','
	        << float_bits(bounds.min.z) << ',' << float_bits(bounds.max.x) << ','
	        << float_bits(bounds.max.y) << ',' << float_bits(bounds.max.z);
}

} // namespace

std::shared_ptr<const VineShapeSpecification>
VineShapeSpecificationValidator::validate(
	VineShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	auto fail = [&](const std::string &message) {
		if (diagnostic != nullptr) *diagnostic = message;
		return std::shared_ptr<const VineShapeSpecification>();
	};
	if (!candidate.species.has_value() ||
	    candidate.species->identifier().empty()) {
		return fail("Vine requires a species data record.");
	}
	if (candidate.species->architecture() != PlantArchitecture::Vine) {
		return fail("Vine species must use the Vine plant architecture.");
	}
	if (!finite(candidate.start_position) || !finite(candidate.initial_direction) ||
	    glm::length(candidate.initial_direction) <= 1.0e-7f ||
	    !std::isfinite(candidate.initial_radius) || candidate.initial_radius <= 0.0f) {
		return fail("Vine start, direction, and initial radius must be finite and non-zero.");
	}
	if (!valid_growth_mode(candidate.growth_mode) ||
	    !valid_collision_behavior(candidate.collision_behavior) ||
	    !valid_attachment_mode(candidate.attachment_mode)) {
		return fail("Vine growth, collision, and attachment modes must be supported values.");
	}
	if (!std::isfinite(candidate.step_length) || candidate.step_length <= 0.0f ||
	    candidate.maximum_segments == 0u ||
	    candidate.maximum_segments + 1u > complexity_limits_.maximumBranchNodes() ||
	    candidate.maximum_segments > complexity_limits_.maximumBranchSegments()) {
		return fail("Vine step length and segment count must be positive and within safety ceilings.");
	}
	if (!std::isfinite(candidate.maximum_seek_distance) ||
	    candidate.maximum_seek_distance < 0.0f ||
	    !std::isfinite(candidate.attachment_distance) ||
	    candidate.attachment_distance < 0.0f ||
	    !std::isfinite(candidate.attachment_tolerance) ||
	    candidate.attachment_tolerance <= 0.0f ||
	    !std::isfinite(candidate.radius_decay) || candidate.radius_decay <= 0.0f ||
	    candidate.radius_decay > 1.0f ||
	    !std::isfinite(candidate.minimum_radius) || candidate.minimum_radius <= 0.0f ||
	    candidate.minimum_radius > candidate.initial_radius ||
	    !finite(candidate.preferred_direction) ||
	    glm::length(candidate.preferred_direction) <= 1.0e-7f ||
	    !std::isfinite(candidate.radius_conservation_exponent) ||
	    candidate.radius_conservation_exponent <= 0.0f) {
		return fail("Vine seek, attachment, radius, direction, and conservation parameters are invalid.");
	}
	if ((target_required(candidate.growth_mode) ||
	     candidate.collision_behavior == VegetationCollisionBehavior::Seek) &&
	    !candidate.target.has_value()) {
		return fail("The selected Vine growth mode requires target(identifier bounds...).");
	}
	if (candidate.target.has_value() &&
	    (candidate.target->identifier().empty() ||
	     candidate.target->geometryKind() !=
		     VegetationSurfaceGeometryKind::AxisAlignedBox ||
	     !finite_bounds(candidate.target->bounds()))) {
		return fail("Vine target must be a named finite axis-aligned surface boundary.");
	}

	std::unordered_set<std::string> obstacle_identifiers;
	for (const VegetationObstacleBoundary &obstacle : candidate.obstacles) {
		if (obstacle.identifier().empty() ||
		    !obstacle_identifiers.insert(obstacle.identifier()).second ||
		    !finite_bounds(obstacle.bounds()) ||
		    !std::isfinite(obstacle.clearance()) || obstacle.clearance() < 0.0f) {
			return fail("Vine obstacles must have unique names, finite bounds, and non-negative clearance.");
		}
		if (candidate.target.has_value() &&
		    candidate.target->identifier() == obstacle.identifier()) {
			return fail("Vine target cannot also be an avoidance obstacle.");
		}
	}

	candidate.initial_direction = glm::normalize(candidate.initial_direction);
	candidate.preferred_direction = glm::normalize(candidate.preferred_direction);
	const VineGrowthSpecification growth(
		candidate.growth_mode, candidate.collision_behavior,
		candidate.attachment_mode, candidate.step_length,
		candidate.maximum_segments, candidate.maximum_seek_distance,
		candidate.attachment_distance, candidate.attachment_tolerance,
		candidate.radius_decay, candidate.minimum_radius,
		candidate.preferred_direction,
		candidate.radius_conservation_exponent);

	std::ostringstream canonical;
	canonical << "Vine(species(" << candidate.species->identifier() << ") start("
	          << candidate.start_position.x << ' ' << candidate.start_position.y << ' '
	          << candidate.start_position.z << ") direction("
	          << candidate.initial_direction.x << ' ' << candidate.initial_direction.y << ' '
	          << candidate.initial_direction.z << ") radius(" << candidate.initial_radius
	          << ") mode(" << vineGrowthModeName(candidate.growth_mode)
	          << ") collision("
	          << vegetationCollisionBehaviorName(candidate.collision_behavior)
	          << ") attachment("
	          << surfaceAttachmentModeName(candidate.attachment_mode)
	          << ") step(" << candidate.step_length << ") segments("
	          << candidate.maximum_segments << ") seekDistance("
	          << candidate.maximum_seek_distance << ") attachDistance("
	          << candidate.attachment_distance << ") tolerance("
	          << candidate.attachment_tolerance << ") radiusDecay("
	          << candidate.radius_decay << ") minimumRadius("
	          << candidate.minimum_radius << ") preferred("
	          << candidate.preferred_direction.x << ' '
	          << candidate.preferred_direction.y << ' '
	          << candidate.preferred_direction.z << ") gamma("
	          << candidate.radius_conservation_exponent << ')';
	if (candidate.target.has_value()) {
		const AxisAlignedBounds &bounds = candidate.target->bounds();
		canonical << " target(" << candidate.target->identifier() << ' '
		          << bounds.min.x << ' ' << bounds.min.y << ' ' << bounds.min.z << ' '
		          << bounds.max.x << ' ' << bounds.max.y << ' ' << bounds.max.z << ')';
	}
	for (const VegetationObstacleBoundary &obstacle : candidate.obstacles) {
		const AxisAlignedBounds &bounds = obstacle.bounds();
		canonical << " obstacle(" << obstacle.identifier() << ' '
		          << bounds.min.x << ' ' << bounds.min.y << ' ' << bounds.min.z << ' '
		          << bounds.max.x << ' ' << bounds.max.y << ' ' << bounds.max.z << ' '
		          << obstacle.clearance() << ')';
	}
	canonical << " detail(" << geometryDetailLevelName(candidate.detail_level) << "))";

	std::ostringstream key;
	key << "Vine:v1:species=" << candidate.species->identifier()
	    << ":start=" << float_bits(candidate.start_position.x) << ','
	    << float_bits(candidate.start_position.y) << ','
	    << float_bits(candidate.start_position.z)
	    << ":direction=" << float_bits(candidate.initial_direction.x) << ','
	    << float_bits(candidate.initial_direction.y) << ','
	    << float_bits(candidate.initial_direction.z)
	    << ":radius=" << float_bits(candidate.initial_radius)
	    << ":mode=" << static_cast<int>(candidate.growth_mode)
	    << ":collision=" << static_cast<int>(candidate.collision_behavior)
	    << ":attachment=" << static_cast<int>(candidate.attachment_mode)
	    << ":step=" << float_bits(candidate.step_length)
	    << ":segments=" << candidate.maximum_segments
	    << ":seek=" << float_bits(candidate.maximum_seek_distance)
	    << ":attach=" << float_bits(candidate.attachment_distance)
	    << ":tolerance=" << float_bits(candidate.attachment_tolerance)
	    << ":decay=" << float_bits(candidate.radius_decay)
	    << ":minimum=" << float_bits(candidate.minimum_radius)
	    << ":preferred=" << float_bits(candidate.preferred_direction.x) << ','
	    << float_bits(candidate.preferred_direction.y) << ','
	    << float_bits(candidate.preferred_direction.z)
	    << ":gamma=" << float_bits(candidate.radius_conservation_exponent);
	if (candidate.target.has_value()) {
		key << ":target=" << candidate.target->identifier() << ':';
		append_bounds(&key, candidate.target->bounds());
	}
	for (const VegetationObstacleBoundary &obstacle : candidate.obstacles) {
		key << ":obstacle=" << obstacle.identifier() << ':';
		append_bounds(&key, obstacle.bounds());
		key << ':' << float_bits(obstacle.clearance());
	}

	if (diagnostic != nullptr) diagnostic->clear();
	return std::make_shared<const VineShapeSpecification>(
		std::move(*candidate.species), candidate.start_position,
		candidate.initial_direction, candidate.initial_radius, growth,
		std::move(candidate.target), std::move(candidate.obstacles),
		ShapeSpecificationKey(key.str()), canonical.str(), candidate.detail_level);
}
