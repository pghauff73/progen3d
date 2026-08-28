#include "vegetation/service/ScatterRegionShapeSpecificationValidator.h"

#include "vegetation/model/ScatterCollisionPolicy.h"
#include "vegetation/model/ScatterRegionRequest.h"
#include "vegetation/service/ScatterRegionService.h"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

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

std::shared_ptr<const ScatterRegionShapeSpecification>
ScatterRegionShapeSpecificationValidator::validate(
	ScatterRegionShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	auto fail = [&](const std::string &message) {
		if (diagnostic != nullptr) *diagnostic = message;
		return std::shared_ptr<const ScatterRegionShapeSpecification>();
	};
	if (!candidate.species.has_value() || candidate.species->identifier().empty()) {
		return fail("ScatterRegion requires a vegetation species data record.");
	}
	if (candidate.region_identifier.empty() || !candidate.surface.has_value()) {
		return fail("ScatterRegion requires region(identifier) and surface(identifier bounds...).");
	}
	ScatterRegionSpecification scatter(
		candidate.region_identifier, candidate.species->identifier(),
		*candidate.surface, candidate.surface_face, candidate.density,
		candidate.minimum_distance, candidate.scale_range,
		candidate.orientation_mode, candidate.collision_radius,
		ScatterCollisionPolicy(
			candidate.placement_layer, candidate.collision_mask));
	const ScatterRegionResult validation = ScatterRegionService(complexity_limits_).resolve(
		ScatterRegionRequest(
			scatter, candidate.deterministic_seed, candidate.obstacles));
	if (!validation.succeeded()) return fail(validation.diagnostic());

	const AxisAlignedBounds &surface_bounds = candidate.surface->bounds();
	std::ostringstream canonical;
	canonical << "ScatterRegion(region(" << candidate.region_identifier
	          << ") species(" << candidate.species->identifier() << ") seed("
	          << candidate.deterministic_seed << ") surface("
	          << candidate.surface->identifier() << ' '
	          << surface_bounds.min.x << ' ' << surface_bounds.min.y << ' '
	          << surface_bounds.min.z << ' ' << surface_bounds.max.x << ' '
	          << surface_bounds.max.y << ' ' << surface_bounds.max.z
	          << ") face(" << scatterSurfaceFaceName(candidate.surface_face)
	          << ") density(" << candidate.density << ") separation("
	          << candidate.minimum_distance << ") scale("
	          << candidate.scale_range.x << ' ' << candidate.scale_range.y
	          << ") orientation("
	          << scatterOrientationModeName(candidate.orientation_mode)
	          << ") collisionRadius(" << candidate.collision_radius
	          << ") layer(" << collisionLayerName(candidate.placement_layer)
	          << ") maskBits(" << candidate.collision_mask.bits() << ')';
	for (const ScatterObstacleBoundary &obstacle : candidate.obstacles) {
		const AxisAlignedBounds &bounds = obstacle.bounds();
		canonical << " obstacle(" << obstacle.identifier() << ' '
		          << bounds.min.x << ' ' << bounds.min.y << ' ' << bounds.min.z << ' '
		          << bounds.max.x << ' ' << bounds.max.y << ' ' << bounds.max.z << ' '
		          << collisionLayerName(obstacle.collisionLayer()) << ')';
	}
	canonical << " detail(" << geometryDetailLevelName(candidate.detail_level)
	          << "))";

	std::ostringstream key;
	key << "ScatterRegion:v1:region=" << candidate.region_identifier
	    << ":species=" << candidate.species->identifier()
	    << ":seed=" << candidate.deterministic_seed
	    << ":surface=" << candidate.surface->identifier() << ':';
	append_bounds(&key, surface_bounds);
	key << ":face=" << static_cast<int>(candidate.surface_face)
	    << ":density=" << float_bits(candidate.density)
	    << ":separation=" << float_bits(candidate.minimum_distance)
	    << ":scale=" << float_bits(candidate.scale_range.x) << ','
	    << float_bits(candidate.scale_range.y)
	    << ":orientation=" << static_cast<int>(candidate.orientation_mode)
	    << ":radius=" << float_bits(candidate.collision_radius)
	    << ":layer=" << static_cast<int>(candidate.placement_layer)
	    << ":mask=" << candidate.collision_mask.bits();
	for (const ScatterObstacleBoundary &obstacle : candidate.obstacles) {
		key << ":obstacle=" << obstacle.identifier() << ':';
		append_bounds(&key, obstacle.bounds());
		key << ':' << static_cast<int>(obstacle.collisionLayer());
	}

	if (diagnostic != nullptr) diagnostic->clear();
	return std::make_shared<const ScatterRegionShapeSpecification>(
		std::move(*candidate.species), std::move(scatter),
		candidate.deterministic_seed, std::move(candidate.obstacles),
		ShapeSpecificationKey(key.str()), canonical.str(), candidate.detail_level);
}
