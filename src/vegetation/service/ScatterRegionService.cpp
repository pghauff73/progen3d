#include "vegetation/service/ScatterRegionService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr float kGeometryTolerance = 1.0e-7f;
constexpr float kPi = 3.14159265358979323846f;

struct ScatterSurfaceFrame
{
	glm::vec3 normal{0.0f, 1.0f, 0.0f};
	glm::vec3 horizontal_axis{1.0f, 0.0f, 0.0f};
	glm::vec3 vertical_axis{0.0f, 0.0f, 1.0f};
	float horizontal_minimum = 0.0f;
	float horizontal_maximum = 0.0f;
	float vertical_minimum = 0.0f;
	float vertical_maximum = 0.0f;
	glm::vec3 fixed_position{0.0f};
};

struct GridCell
{
	long long horizontal = 0;
	long long vertical = 0;

	bool operator==(const GridCell &other) const
	{
		return horizontal == other.horizontal && vertical == other.vertical;
	}
};

struct GridCellHash
{
	std::size_t operator()(const GridCell &cell) const
	{
		const std::uint64_t horizontal =
			static_cast<std::uint64_t>(cell.horizontal);
		const std::uint64_t vertical =
			static_cast<std::uint64_t>(cell.vertical);
		return static_cast<std::size_t>(
			(horizontal * 0x9e3779b97f4a7c15ull) ^
			(vertical + 0x517cc1b727220a95ull));
	}
};

struct AcceptedScatterPlacement
{
	ScatterPlacement placement;
	glm::vec2 surface_coordinate{0.0f};
};

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite_bounds(const AxisAlignedBounds &bounds)
{
	return bounds.valid && finite(bounds.min) && finite(bounds.max) &&
	       bounds.min.x <= bounds.max.x &&
	       bounds.min.y <= bounds.max.y &&
	       bounds.min.z <= bounds.max.z;
}

bool point_inside_bounds(const glm::vec3 &point, const AxisAlignedBounds &bounds)
{
	return point.x >= bounds.min.x && point.x <= bounds.max.x &&
	       point.y >= bounds.min.y && point.y <= bounds.max.y &&
	       point.z >= bounds.min.z && point.z <= bounds.max.z;
}

AxisAlignedBounds expanded_bounds(const AxisAlignedBounds &bounds, float amount)
{
	AxisAlignedBounds expanded = bounds;
	expanded.min -= glm::vec3(amount);
	expanded.max += glm::vec3(amount);
	expanded.center = (expanded.min + expanded.max) * 0.5f;
	expanded.half_extents = (expanded.max - expanded.min) * 0.5f;
	return expanded;
}

bool surface_face_is_valid(ScatterSurfaceFace face)
{
	switch (face) {
	case ScatterSurfaceFace::MinimumX:
	case ScatterSurfaceFace::MaximumX:
	case ScatterSurfaceFace::MinimumY:
	case ScatterSurfaceFace::MaximumY:
	case ScatterSurfaceFace::MinimumZ:
	case ScatterSurfaceFace::MaximumZ:
		return true;
	}
	return false;
}

bool orientation_mode_is_valid(ScatterOrientationMode mode)
{
	switch (mode) {
	case ScatterOrientationMode::SurfaceNormal:
	case ScatterOrientationMode::SurfaceNormalRandomAzimuth:
	case ScatterOrientationMode::WorldUpRandomAzimuth:
		return true;
	}
	return false;
}

bool collision_layer_is_valid(CollisionLayer layer)
{
	switch (layer) {
	case CollisionLayer::Structure:
	case CollisionLayer::Envelope:
	case CollisionLayer::Interior:
	case CollisionLayer::Furniture:
	case CollisionLayer::Plumbing:
	case CollisionLayer::Hvac:
	case CollisionLayer::Electrical:
	case CollisionLayer::Equipment:
	case CollisionLayer::Terrain:
	case CollisionLayer::Temporary:
		return true;
	}
	return false;
}

ScatterSurfaceFrame surface_frame(
	const AxisAlignedBounds &bounds,
	ScatterSurfaceFace face)
{
	ScatterSurfaceFrame frame;
	switch (face) {
	case ScatterSurfaceFace::MinimumX:
		frame.normal = glm::vec3(-1.0f, 0.0f, 0.0f);
		frame.horizontal_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		frame.horizontal_minimum = bounds.min.y;
		frame.horizontal_maximum = bounds.max.y;
		frame.vertical_minimum = bounds.min.z;
		frame.vertical_maximum = bounds.max.z;
		frame.fixed_position.x = bounds.min.x;
		break;
	case ScatterSurfaceFace::MaximumX:
		frame.normal = glm::vec3(1.0f, 0.0f, 0.0f);
		frame.horizontal_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		frame.horizontal_minimum = bounds.min.y;
		frame.horizontal_maximum = bounds.max.y;
		frame.vertical_minimum = bounds.min.z;
		frame.vertical_maximum = bounds.max.z;
		frame.fixed_position.x = bounds.max.x;
		break;
	case ScatterSurfaceFace::MinimumY:
		frame.normal = glm::vec3(0.0f, -1.0f, 0.0f);
		frame.horizontal_axis = glm::vec3(1.0f, 0.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		frame.horizontal_minimum = bounds.min.x;
		frame.horizontal_maximum = bounds.max.x;
		frame.vertical_minimum = bounds.min.z;
		frame.vertical_maximum = bounds.max.z;
		frame.fixed_position.y = bounds.min.y;
		break;
	case ScatterSurfaceFace::MaximumY:
		frame.normal = glm::vec3(0.0f, 1.0f, 0.0f);
		frame.horizontal_axis = glm::vec3(1.0f, 0.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 0.0f, 1.0f);
		frame.horizontal_minimum = bounds.min.x;
		frame.horizontal_maximum = bounds.max.x;
		frame.vertical_minimum = bounds.min.z;
		frame.vertical_maximum = bounds.max.z;
		frame.fixed_position.y = bounds.max.y;
		break;
	case ScatterSurfaceFace::MinimumZ:
		frame.normal = glm::vec3(0.0f, 0.0f, -1.0f);
		frame.horizontal_axis = glm::vec3(1.0f, 0.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		frame.horizontal_minimum = bounds.min.x;
		frame.horizontal_maximum = bounds.max.x;
		frame.vertical_minimum = bounds.min.y;
		frame.vertical_maximum = bounds.max.y;
		frame.fixed_position.z = bounds.min.z;
		break;
	case ScatterSurfaceFace::MaximumZ:
		frame.normal = glm::vec3(0.0f, 0.0f, 1.0f);
		frame.horizontal_axis = glm::vec3(1.0f, 0.0f, 0.0f);
		frame.vertical_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		frame.horizontal_minimum = bounds.min.x;
		frame.horizontal_maximum = bounds.max.x;
		frame.vertical_minimum = bounds.min.y;
		frame.vertical_maximum = bounds.max.y;
		frame.fixed_position.z = bounds.max.z;
		break;
	}
	return frame;
}

glm::vec3 surface_position(
	const ScatterSurfaceFrame &frame,
	float horizontal,
	float vertical)
{
	return frame.fixed_position +
	       frame.horizontal_axis * horizontal +
	       frame.vertical_axis * vertical;
}

double halton(std::uint64_t index, std::uint64_t base)
{
	double fraction = 1.0;
	double result = 0.0;
	while (index > 0u) {
		fraction /= static_cast<double>(base);
		result += fraction * static_cast<double>(index % base);
		index /= base;
	}
	return result;
}

std::uint64_t mixed_seed(std::uint64_t seed)
{
	seed ^= seed >> 30u;
	seed *= 0xbf58476d1ce4e5b9ull;
	seed ^= seed >> 27u;
	seed *= 0x94d049bb133111ebull;
	seed ^= seed >> 31u;
	return seed;
}

glm::mat4 placement_transform(
	const glm::vec3 &position,
	const ScatterSurfaceFrame &frame,
	ScatterOrientationMode orientation_mode,
	float azimuth_degrees,
	float scale)
{
	glm::vec3 up = frame.normal;
	glm::vec3 right = frame.horizontal_axis;
	if (orientation_mode == ScatterOrientationMode::WorldUpRandomAzimuth) {
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		right = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	glm::vec3 forward = glm::normalize(glm::cross(right, up));
	if (glm::length(forward) <= kGeometryTolerance) {
		forward = glm::vec3(0.0f, 0.0f, 1.0f);
	}
	const float radians = azimuth_degrees * kPi / 180.0f;
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	const glm::vec3 rotated_right = right * cosine - forward * sine;
	const glm::vec3 rotated_forward = right * sine + forward * cosine;
	glm::mat4 transform(1.0f);
	transform[0] = glm::vec4(rotated_right * scale, 0.0f);
	transform[1] = glm::vec4(up * scale, 0.0f);
	transform[2] = glm::vec4(rotated_forward * scale, 0.0f);
	transform[3] = glm::vec4(position, 1.0f);
	return transform;
}

GridCell grid_cell(glm::vec2 coordinate, float cell_size)
{
	return GridCell{
		static_cast<long long>(std::floor(coordinate.x / cell_size)),
		static_cast<long long>(std::floor(coordinate.y / cell_size))};
}

bool violates_spacing(
	glm::vec2 candidate,
	float minimum_distance,
	const std::vector<AcceptedScatterPlacement> &placements,
	const std::unordered_map<GridCell, std::vector<std::size_t>, GridCellHash> &grid)
{
	if (minimum_distance <= 0.0f) return false;
	const GridCell cell = grid_cell(candidate, minimum_distance);
	const float minimum_distance_squared = minimum_distance * minimum_distance;
	for (long long horizontal_offset = -1; horizontal_offset <= 1;
	     ++horizontal_offset) {
		for (long long vertical_offset = -1; vertical_offset <= 1;
		     ++vertical_offset) {
			const GridCell neighbor{
				cell.horizontal + horizontal_offset,
				cell.vertical + vertical_offset};
			const auto found = grid.find(neighbor);
			if (found == grid.end()) continue;
			for (std::size_t placement_index : found->second) {
				const glm::vec2 difference =
					placements[placement_index].surface_coordinate - candidate;
				if (glm::dot(difference, difference) < minimum_distance_squared) {
					return true;
				}
			}
		}
	}
	return false;
}

bool collides(
	const glm::vec3 &position,
	float collision_radius,
	float scale,
	const ScatterCollisionPolicy &policy,
	const std::vector<ScatterObstacleBoundary> &obstacles)
{
	const float effective_radius = collision_radius * scale;
	for (const ScatterObstacleBoundary &obstacle : obstacles) {
		if (!policy.collisionMask().contains(obstacle.collisionLayer())) continue;
		if (point_inside_bounds(
				position, expanded_bounds(obstacle.bounds(), effective_radius))) {
			return true;
		}
	}
	return false;
}

void hash_bytes(std::uint64_t *hash, const void *data, std::size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	for (std::size_t index = 0; index < size; ++index) {
		*hash ^= bytes[index];
		*hash *= 1099511628211ull;
	}
}

void hash_string(std::uint64_t *hash, const std::string &value)
{
	hash_bytes(hash, value.data(), value.size());
	const unsigned char separator = 0xffu;
	hash_bytes(hash, &separator, sizeof(separator));
}

void hash_vector(std::uint64_t *hash, const glm::vec3 &value)
{
	hash_bytes(hash, &value.x, sizeof(float));
	hash_bytes(hash, &value.y, sizeof(float));
	hash_bytes(hash, &value.z, sizeof(float));
}

std::uint64_t evidence_hash(
	const ScatterRegionRequest &request,
	const std::vector<ScatterPlacement> &placements,
	std::size_t requested_placement_count,
	std::size_t evaluated_candidate_count,
	std::size_t spacing_rejection_count,
	std::size_t collision_rejection_count)
{
	std::uint64_t value = 1469598103934665603ull;
	const ScatterRegionSpecification &specification = request.specification();
	hash_string(&value, specification.identifier());
	hash_string(&value, specification.objectIdentifier());
	hash_string(&value, specification.surface().identifier());
	const int geometry_kind =
		static_cast<int>(specification.surface().geometryKind());
	const int face = static_cast<int>(specification.surfaceFace());
	const int orientation = static_cast<int>(specification.orientationMode());
	const int placement_layer =
		static_cast<int>(specification.collisionPolicy().placementLayer());
	hash_bytes(&value, &geometry_kind, sizeof(geometry_kind));
	hash_bytes(&value, &face, sizeof(face));
	hash_bytes(&value, &orientation, sizeof(orientation));
	hash_bytes(&value, &placement_layer, sizeof(placement_layer));
	const std::uint32_t collision_mask =
		specification.collisionPolicy().collisionMask().bits();
	hash_bytes(&value, &collision_mask, sizeof(collision_mask));
	hash_vector(&value, specification.surface().bounds().min);
	hash_vector(&value, specification.surface().bounds().max);
	for (const float scalar : {
		 specification.density(), specification.minimumDistance(),
		 specification.scaleRange().x, specification.scaleRange().y,
		 specification.collisionRadius()}) {
		hash_bytes(&value, &scalar, sizeof(scalar));
	}
	const std::uint64_t seed = request.deterministicSeed();
	hash_bytes(&value, &seed, sizeof(seed));
	for (const ScatterObstacleBoundary &obstacle : request.obstacles()) {
		hash_string(&value, obstacle.identifier());
		hash_vector(&value, obstacle.bounds().min);
		hash_vector(&value, obstacle.bounds().max);
		const int layer = static_cast<int>(obstacle.collisionLayer());
		hash_bytes(&value, &layer, sizeof(layer));
	}
	for (const ScatterPlacement &placement : placements) {
		hash_string(&value, placement.identifier());
		hash_vector(&value, placement.position());
		hash_vector(&value, placement.surfaceNormal());
		const float scale = placement.scale();
		const float azimuth = placement.azimuthDegrees();
		hash_bytes(&value, &scale, sizeof(scale));
		hash_bytes(&value, &azimuth, sizeof(azimuth));
	}
	hash_bytes(
		&value, &requested_placement_count,
		sizeof(requested_placement_count));
	hash_bytes(
		&value, &evaluated_candidate_count,
		sizeof(evaluated_candidate_count));
	hash_bytes(
		&value, &spacing_rejection_count,
		sizeof(spacing_rejection_count));
	hash_bytes(
		&value, &collision_rejection_count,
		sizeof(collision_rejection_count));
	return value;
}

} // namespace

ScatterRegionResult ScatterRegionService::resolve(
	const ScatterRegionRequest &request) const
{
	const ScatterRegionSpecification &specification = request.specification();
	auto fail = [](const std::string &diagnostic) {
		return ScatterRegionResult::failed(diagnostic);
	};
	if (specification.identifier().empty() ||
	    specification.objectIdentifier().empty() ||
	    specification.surface().identifier().empty()) {
		return fail(
			"ScatterRegion requires region, object, and surface identifiers.");
	}
	if (specification.surface().geometryKind() !=
	    VegetationSurfaceGeometryKind::AxisAlignedBox) {
		return fail(
			"ScatterRegion supports AxisAlignedBox surfaces in V1D; "
			"TriangleMesh surfaces fail closed.");
	}
	if (!finite_bounds(specification.surface().bounds()) ||
	    !surface_face_is_valid(specification.surfaceFace()) ||
	    !orientation_mode_is_valid(specification.orientationMode()) ||
	    !collision_layer_is_valid(
		    specification.collisionPolicy().placementLayer())) {
		return fail("ScatterRegion contains invalid surface or policy values.");
	}
	if (!std::isfinite(specification.density()) ||
	    specification.density() <= 0.0f ||
	    !std::isfinite(specification.minimumDistance()) ||
	    specification.minimumDistance() < 0.0f ||
	    !std::isfinite(specification.scaleRange().x) ||
	    !std::isfinite(specification.scaleRange().y) ||
	    specification.scaleRange().x <= 0.0f ||
	    specification.scaleRange().y < specification.scaleRange().x ||
	    !std::isfinite(specification.collisionRadius()) ||
	    specification.collisionRadius() < 0.0f) {
		return fail(
			"ScatterRegion requires finite density, spacing, scale, and "
			"collision-radius values.");
	}

	std::unordered_set<std::string> obstacle_identifiers;
	for (const ScatterObstacleBoundary &obstacle : request.obstacles()) {
		if (obstacle.identifier().empty() ||
		    !obstacle_identifiers.insert(obstacle.identifier()).second ||
		    !finite_bounds(obstacle.bounds()) ||
		    !collision_layer_is_valid(obstacle.collisionLayer())) {
			return fail(
				"ScatterRegion obstacles must have unique identifiers, valid "
				"bounds, and supported collision layers.");
		}
	}

	const ScatterSurfaceFrame frame = surface_frame(
		specification.surface().bounds(), specification.surfaceFace());
	const double horizontal_length = static_cast<double>(
		frame.horizontal_maximum - frame.horizontal_minimum);
	const double vertical_length = static_cast<double>(
		frame.vertical_maximum - frame.vertical_minimum);
	const double area = horizontal_length * vertical_length;
	if (!std::isfinite(area) || area <= kGeometryTolerance) {
		return fail("ScatterRegion selected surface has zero or invalid area.");
	}
	const double requested_measure =
		area * static_cast<double>(specification.density());
	if (!std::isfinite(requested_measure) || requested_measure < 1.0) {
		return fail(
			"ScatterRegion density produces no placements on the selected surface.");
	}
	if (requested_measure >
	    static_cast<double>(complexity_limits_.maximumScatterPlacements())) {
		return fail("ScatterRegion exceeds the placement complexity ceiling.");
	}
	const std::size_t requested_placement_count =
		static_cast<std::size_t>(std::floor(requested_measure));
	if (requested_placement_count == 0u ||
	    requested_placement_count >
		    complexity_limits_.maximumScatterPlacements()) {
		return fail("ScatterRegion exceeds the placement complexity ceiling.");
	}

	std::vector<AcceptedScatterPlacement> accepted;
	accepted.reserve(requested_placement_count);
	std::unordered_map<GridCell, std::vector<std::size_t>, GridCellHash> grid;
	std::size_t spacing_rejection_count = 0;
	std::size_t collision_rejection_count = 0;
	std::size_t evaluated_candidate_count = 0;
	const std::size_t maximum_attempts =
		requested_placement_count * 64u + 1024u;
	const std::uint64_t start_index =
		1u + mixed_seed(request.deterministicSeed()) % 104729u;
	for (std::size_t attempt = 0;
	     attempt < maximum_attempts &&
	     accepted.size() < requested_placement_count;
	     ++attempt) {
		++evaluated_candidate_count;
		const std::uint64_t sequence_index =
			start_index + static_cast<std::uint64_t>(attempt);
		const float horizontal_unit =
			static_cast<float>(halton(sequence_index, 2u));
		const float vertical_unit =
			static_cast<float>(halton(sequence_index, 3u));
		const glm::vec2 surface_coordinate(
			frame.horizontal_minimum +
				horizontal_unit *
				(frame.horizontal_maximum - frame.horizontal_minimum),
			frame.vertical_minimum +
				vertical_unit *
				(frame.vertical_maximum - frame.vertical_minimum));
		if (violates_spacing(
				surface_coordinate, specification.minimumDistance(),
				accepted, grid)) {
			++spacing_rejection_count;
			continue;
		}
		const float scale_unit =
			static_cast<float>(halton(sequence_index, 5u));
		const float scale = specification.scaleRange().x +
			scale_unit *
			(specification.scaleRange().y - specification.scaleRange().x);
		const glm::vec3 position = surface_position(
			frame, surface_coordinate.x, surface_coordinate.y);
		if (collides(
				position, specification.collisionRadius(), scale,
				specification.collisionPolicy(), request.obstacles())) {
			++collision_rejection_count;
			continue;
		}
		float azimuth_degrees = 0.0f;
		if (specification.orientationMode() !=
		    ScatterOrientationMode::SurfaceNormal) {
			azimuth_degrees =
				static_cast<float>(halton(sequence_index, 7u)) * 360.0f;
		}
		const std::string placement_identifier =
			specification.identifier() + "_placement_" +
			std::to_string(accepted.size());
		const ScatterPlacement placement(
			placement_identifier, specification.objectIdentifier(),
			specification.surface().identifier(), position, frame.normal,
			scale, azimuth_degrees,
			placement_transform(
				position, frame, specification.orientationMode(),
				azimuth_degrees, scale));
		const std::size_t accepted_index = accepted.size();
		accepted.push_back(AcceptedScatterPlacement{placement, surface_coordinate});
		if (specification.minimumDistance() > 0.0f) {
			grid[grid_cell(
				surface_coordinate, specification.minimumDistance())]
				.push_back(accepted_index);
		}
	}
	if (accepted.empty()) {
		return fail(
			"ScatterRegion could not place any object without violating "
			"spacing or collision constraints.");
	}

	std::vector<ScatterPlacement> placements;
	placements.reserve(accepted.size());
	for (AcceptedScatterPlacement &entry : accepted) {
		placements.push_back(std::move(entry.placement));
	}
	const std::uint64_t resolved_evidence_hash = evidence_hash(
		request, placements, requested_placement_count,
		evaluated_candidate_count, spacing_rejection_count,
		collision_rejection_count);
	const std::size_t accepted_placement_count = placements.size();
	return ScatterRegionResult::succeeded(ScatterRegionSnapshot(
		std::move(placements),
		ScatterRegionResolutionEvidence(
			request.deterministicSeed(), resolved_evidence_hash,
			requested_placement_count, accepted_placement_count,
			evaluated_candidate_count, spacing_rejection_count,
			collision_rejection_count)));
}
