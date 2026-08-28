#include "Context.h"

#include "PLYWriter.h"
#include "building/generated/SmallModernBuildingKnowledgeCatalog.h"
#include "building/model/SmallModernBuildingModel.h"
#include "building/service/SmallModernBuildingModelConstructionService.h"
#include "geometry/model/ShapeSpecification.h"
#include "spatial/model/CollisionParticipationPolicy.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialConnection.h"
#include "spatial/model/SpatialConstraint.h"
#include "spatial/model/SpatialFrameState.h"
#include "spatial/model/SpatialInterface.h"
#include "spatial/model/SpatialObjectIdentity.h"
#include "spatial/model/SpatialObjectState.h"
#include "spatial/relationship/SpatialContainmentRelationship.h"
#include "spatial/service/SpatialAssemblyResolutionService.h"
#include "spatial/service/SpatialBuildingModelConstructionContext.h"
#include "lighting/model/LightingSceneDefinition.h"
#include "geometry/service/PrimitiveGeometryResolver.h"
#include "physics/model/ConvexCollisionShape.h"
#include "physics/service/ConvexCollisionDetector.h"
#include "vehicle/model/VehicleJointGraph.h"
#include "vehicle/service/VehicleKinematicValidationService.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <random>
#include <stack>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include <glm/ext.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

extern void draw_box(glm::vec3 scale_vec,
                     glm::vec3 position_vec,
                     glm::vec3 pos,
                     int tex_index,
                     float texscale,
                     float alpha);
std::array<GLfloat, 36 * 8> build_base_vertex_data();
extern void errorout(std::string error_str);

class SceneGenerationContext::SpatialSceneConstructionState {
public:
	struct ActiveObjectScope {
		SpatialObjectId object_id;
		glm::mat4 authored_world_transform{1.0f};
	};

	SpatialBuildingModelConstructionContext construction_context;
	std::vector<ActiveObjectScope> active_object_scopes;
	std::unordered_map<std::string, glm::mat4> declared_world_transforms;
	std::shared_ptr<const SpatialBuildingModel> resolved_model;
	std::shared_ptr<const SmallModernBuildingModel> building_model;
	std::size_t next_instance_index = 0;
	bool declarations_started = false;
	std::string blocking_diagnostic;
};
extern std::mt19937_64 effects_rng;

namespace {

constexpr int kPrimitiveVertexCount = 36;
constexpr int kVertexStride = 8;
constexpr float kDefaultMass = 1.0f;
constexpr float kMinimumMass = 0.001f;
constexpr float kCollisionRestitution = 0.65f;
constexpr float kRestingContactImpactSpeed = 1.0f;
constexpr float kPenetrationCorrection = 0.8f;
constexpr float kPenetrationSlop = 0.0005f;
constexpr float kCollisionWakePenetration = 0.01f;
constexpr float kCollisionDistanceRejectionFactor = 1.0f;
constexpr float kMinDelta = 0.000001f;
constexpr float kSimulationBoundsExtent = 100.0f;
constexpr std::size_t kMaxGeneratedPrimitiveCount = 50000;
constexpr std::size_t kMaxCollisionParticles = 4096;
constexpr int kCollisionParticleMinBurst = 18;
constexpr int kCollisionParticleMaxBurst = 40;
constexpr float kCollisionParticleMinSpeed = 0.45f;
constexpr float kCollisionParticleMaxSpeed = 2.35f;
constexpr float kCollisionParticleGravityScale = 0.35f;
constexpr float kCollisionParticleDamping = 0.965f;
constexpr float kCollisionParticleMinLifetime = 0.18f;
constexpr float kCollisionParticleMaxLifetime = 0.52f;
constexpr float kSleepLinearSpeed = 0.045f;
constexpr float kSleepAngularSpeed = 7.5f;
constexpr float kWakeLinearSpeed = 0.12f;
constexpr float kWakeAngularSpeed = 15.0f;
constexpr float kSleepDelay = 0.35f;
constexpr float kMaxRotationalSpeed = 720.0f;
constexpr float kBroadPhaseCellSizeMin = 0.5f;
constexpr float kBroadPhaseCellSizeMax = 4.0f;
constexpr float kBroadPhaseCellScale = 1.5f;

using glm::mat4;
using glm::mat3;
using glm::vec2;
using glm::vec3;
using glm::vec4;

std::string canonicalize_material_name(std::string material_name)
{
	std::string canonical;
	canonical.reserve(material_name.size());
	for (unsigned char character : material_name) {
		if (std::isalnum(character) != 0) {
			canonical.push_back(static_cast<char>(std::tolower(character)));
		}
	}

	const std::string user_texture_prefix = "usertexture";
	const bool is_user_texture_slot =
		canonical.size() > user_texture_prefix.size() &&
		canonical.rfind(user_texture_prefix, 0) == 0 &&
		std::all_of(canonical.begin() + static_cast<std::ptrdiff_t>(user_texture_prefix.size()),
		            canonical.end(),
		            [](unsigned char character) {
			            return std::isdigit(character) != 0;
		            });
	if (is_user_texture_slot) {
		return canonical;
	}

	// Always generate a texture material name, no matter what the input is
	if (canonical.empty()) {
		return "softwhitematteplaster";
	}

	// Ensure the material name always has a texture suffix if it doesn't contain one
	const std::vector<std::string> texture_suffixes = {
		"plaster", "paint", "wood", "metal", "glass", "stone", "brick",
		"fabric", "carpet", "tile", "marble", "granite", "concrete"
	};

	bool has_texture_suffix = false;
	for (const auto& suffix : texture_suffixes) {
		if (canonical.find(suffix) != std::string::npos) {
			has_texture_suffix = true;
			break;
		}
	}

	// If no texture suffix is found, append one to ensure texture generation
	if (!has_texture_suffix) {
		canonical += "plaster"; // Default to plaster material for non-texture inputs
	}

	return canonical;
}

bool is_cube_like(const std::string &type)
{
	return type == "Cube" || type == "CubeX" || type == "CubeY" || type == "CubeZ";
}

bool uses_dynamic_cube_preview_path(const PrimitiveInstance &instance)
{
	return !instance.immovable && is_cube_like(instance.type);
}

enum class PrimitiveTransformMode {
	Default,
	CubeX,
	CubeY,
	CubeZ
};

struct PrimitiveTransformCache {
	PrimitiveTransformMode mode = PrimitiveTransformMode::Default;
	std::string type;
	glm::mat4 primary_transform{1.0f};
	glm::mat4 secondary_transform{1.0f};
	glm::mat3 primary_normal_transform{1.0f};
	glm::mat3 secondary_normal_transform{1.0f};
	std::array<glm::vec3, 3> dual_scales{glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
	std::array<glm::vec3, 3> dual_translations{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
};

struct PrimitiveLogicalBounds {
	glm::vec3 min{0.0f};
	glm::vec3 max{0.0f};
	glm::vec3 actual_to_logical_offset{0.0f};
};

struct SpatialHashCell {
	int x = 0;
	int y = 0;
	int z = 0;

	bool operator==(const SpatialHashCell &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct SpatialHashCellHasher {
	std::size_t operator()(const SpatialHashCell &cell) const
	{
		std::size_t seed = static_cast<std::size_t>(static_cast<uint32_t>(cell.x));
		seed ^= static_cast<std::size_t>(static_cast<uint32_t>(cell.y)) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
		seed ^= static_cast<std::size_t>(static_cast<uint32_t>(cell.z)) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
		return seed;
	}
};

struct BroadPhasePair {
	std::size_t a = 0;
	std::size_t b = 0;
};

struct BroadPhasePairKey {
	std::size_t a = 0;
	std::size_t b = 0;

	bool operator==(const BroadPhasePairKey &other) const
	{
		return a == other.a && b == other.b;
	}
};

struct BroadPhasePairKeyHasher {
	std::size_t operator()(const BroadPhasePairKey &key) const
	{
		std::size_t seed = key.a;
		seed ^= key.b + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
		return seed;
	}
};

glm::vec3 transform_scale_magnitudes(const glm::mat4 &transform)
{
	return glm::vec3(glm::length(glm::vec3(transform[0])),
	                 glm::length(glm::vec3(transform[1])),
	                 glm::length(glm::vec3(transform[2])));
}

float normalized_transform_angle(float angle)
{
	angle = std::fmod(angle, 360.0f);
	return angle < 0.0f ? angle + 360.0f : angle;
}

glm::vec3 transform_euler_degrees(const glm::mat4 &transform)
{
	constexpr float epsilon = 1.0e-7f;
	const glm::vec3 raw_x(transform[0]);
	const glm::vec3 raw_y(transform[1]);
	const glm::vec3 raw_z(transform[2]);
	const auto safe_normalize = [](const glm::vec3 &value,
	                                     const glm::vec3 &fallback) {
		const float squared_length = glm::dot(value, value);
		return std::isfinite(squared_length) && squared_length > epsilon
			       ? value / std::sqrt(squared_length)
			       : fallback;
	};

	glm::vec3 x_axis = safe_normalize(raw_x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 y_rejected = raw_y - x_axis * glm::dot(raw_y, x_axis);
	if(glm::dot(y_rejected, y_rejected) <= epsilon){
		const glm::vec3 candidate = std::fabs(x_axis.x) < 0.75f
			                            ? glm::vec3(1.0f, 0.0f, 0.0f)
			                            : glm::vec3(0.0f, 1.0f, 0.0f);
		y_rejected = glm::cross(x_axis, candidate);
	}
	glm::vec3 y_axis = safe_normalize(y_rejected, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 z_axis = safe_normalize(glm::cross(x_axis, y_axis),
	                                 glm::vec3(0.0f, 0.0f, 1.0f));
	if(glm::dot(z_axis, raw_z) < 0.0f)z_axis = -z_axis;
	y_axis = safe_normalize(glm::cross(z_axis, x_axis), y_axis);

	glm::quat orientation = glm::quat_cast(glm::mat3(x_axis, y_axis, z_axis));
	const float orientation_length = glm::length(orientation);
	if(!std::isfinite(orientation_length) || orientation_length <= epsilon){
		return glm::vec3(0.0f);
	}
	orientation /= orientation_length;
	const glm::vec3 euler = glm::degrees(glm::eulerAngles(orientation));
	return glm::vec3(normalized_transform_angle(euler.x),
	                 normalized_transform_angle(euler.y),
	                 normalized_transform_angle(euler.z));
}

glm::mat3 safe_normal_transform(const glm::mat4 &transform)
{
	const glm::mat3 linear(transform);
	const float determinant = glm::determinant(linear);
	if (std::fabs(determinant) <= kMinDelta) {
		return glm::mat3(1.0f);
	}
	return glm::transpose(glm::inverse(linear));
}

PrimitiveTransformCache build_transform_cache(const PrimitiveInstance &instance,
                                             bool compute_normal_transforms = true)
{
	PrimitiveTransformCache cache;
	cache.type = instance.type;
	cache.primary_transform = instance.primary_transform;
	cache.secondary_transform = instance.secondary_transform;
	if (compute_normal_transforms) {
		cache.primary_normal_transform = safe_normal_transform(cache.primary_transform);
		cache.secondary_normal_transform = safe_normal_transform(cache.secondary_transform);
	}
	cache.dual_scales = instance.dual_scales;
	cache.dual_translations = instance.dual_translations;

	if (instance.type == "CubeX") {
		cache.mode = PrimitiveTransformMode::CubeX;
	} else if (instance.type == "CubeY") {
		cache.mode = PrimitiveTransformMode::CubeY;
	} else if (instance.type == "CubeZ") {
		cache.mode = PrimitiveTransformMode::CubeZ;
	}

	return cache;
}

bool can_collide(const PrimitiveInstance &instance)
{
	if (is_cube_like(instance.type)) return true;
	if (!instance.resolved_geometry) return false;
	switch (instance.resolved_geometry->collisionPolicy()) {
	case PrimitiveCollisionPolicy::Disabled:
		return false;
	case PrimitiveCollisionPolicy::StaticTriangleMesh:
		return instance.immovable;
	case PrimitiveCollisionPolicy::ConvexMesh:
		return true;
	}
	return false;
}

bool is_dynamic_simulation_active(const PrimitiveInstance &instance)
{
	return !instance.removed && !instance.immovable && instance.simulation_active && !instance.sleeping;
}

bool is_collision_candidate(const PrimitiveInstance &instance)
{
	return !instance.removed && can_collide(instance) && (instance.immovable || instance.simulation_active);
}

float bounds_extent_metric(const PrimitiveBounds &bounds)
{
	if (!bounds.valid) {
		return 0.0f;
	}
	const vec3 full_extents = bounds.half_extents * 2.0f;
	return std::max(full_extents.x, std::max(full_extents.y, full_extents.z));
}

bool is_below_sleep_threshold(const PrimitiveInstance &instance)
{
	return glm::dot(instance.velocity, instance.velocity) <= (kSleepLinearSpeed * kSleepLinearSpeed) &&
	       glm::dot(instance.rotational_velocity, instance.rotational_velocity) <=
		       (kSleepAngularSpeed * kSleepAngularSpeed);
}

bool exceeds_wake_threshold(const PrimitiveInstance &instance)
{
	return glm::dot(instance.velocity, instance.velocity) >= (kWakeLinearSpeed * kWakeLinearSpeed) ||
	       glm::dot(instance.rotational_velocity, instance.rotational_velocity) >=
		       (kWakeAngularSpeed * kWakeAngularSpeed);
}

PrimitiveLogicalBounds logical_bounds_for_type(const std::string &type)
{
	if (type == "CubeX") {
		return {vec3(0.0f, -0.5f, -0.5f), vec3(1.0f, 0.5f, 0.5f), vec3(0.5f, -0.5f, 0.0f)};
	}
	if (type == "CubeY") {
		return {vec3(-0.5f, 0.0f, -0.5f), vec3(0.5f, 1.0f, 0.5f), vec3(0.0f, 0.0f, 0.0f)};
	}
	if (type == "CubeZ") {
		return {vec3(-0.5f, -0.5f, 0.0f), vec3(0.5f, 0.5f, 1.0f), vec3(0.0f, -0.5f, 0.5f)};
	}
	return {vec3(-0.5f, -0.5f, -0.5f), vec3(0.5f, 0.5f, 0.5f), vec3(0.0f, -0.5f, 0.0f)};
}

bool is_on_min_face(float value, float min_value)
{
	return std::fabs(value - min_value) <= 0.0001f;
}

vec3 to_logical_local_position(const PrimitiveLogicalBounds &bounds, const vec3 &position)
{
	return position + bounds.actual_to_logical_offset;
}

vec3 to_render_local_position(const PrimitiveLogicalBounds &bounds, const vec3 &position)
{
	(void)bounds;
	return position;
}

mat4 build_dual_axis_transform(const PrimitiveTransformCache &cache, int axis)
{
	mat4 transform(1.0f);
	transform = glm::translate(transform, cache.dual_translations[static_cast<std::size_t>(axis)]);
	transform = glm::scale(transform, cache.dual_scales[static_cast<std::size_t>(axis)]);
	return transform;
}

mat4 build_local_dual_transform(const PrimitiveTransformCache &cache,
                                const PrimitiveLogicalBounds &bounds,
                                const vec3 &logical_position)
{
	mat4 transform(1.0f);
	if (is_on_min_face(logical_position.x, bounds.min.x)) {
		transform = build_dual_axis_transform(cache, 0) * transform;
	}
	if (is_on_min_face(logical_position.y, bounds.min.y)) {
		transform = build_dual_axis_transform(cache, 1) * transform;
	}
	if (is_on_min_face(logical_position.z, bounds.min.z)) {
		transform = build_dual_axis_transform(cache, 2) * transform;
	}
	return transform;
}

vec4 transform_local_vertex(const PrimitiveTransformCache &cache,
                            const vec4 &base_vertex,
                            mat3 *local_normal_transform_out = nullptr)
{
	const PrimitiveLogicalBounds bounds = logical_bounds_for_type(cache.type);
	const vec3 logical_position = to_logical_local_position(bounds, vec3(base_vertex));
	const mat4 local_transform = build_local_dual_transform(cache, bounds, logical_position);
	if (local_normal_transform_out != nullptr) {
		*local_normal_transform_out = safe_normal_transform(local_transform);
	}
	const vec4 transformed_logical = local_transform * vec4(logical_position, 1.0f);
	return vec4(to_render_local_position(bounds, vec3(transformed_logical)), 1.0f);
}

void select_world_transform(const PrimitiveTransformCache &cache,
                            const vec4 &base_vertex,
                            const mat4 **transform_out,
                            const mat3 **normal_transform_out)
{
	if (transform_out == nullptr || normal_transform_out == nullptr) {
		return;
	}

	*transform_out = &cache.primary_transform;
	*normal_transform_out = &cache.primary_normal_transform;

	const PrimitiveLogicalBounds bounds = logical_bounds_for_type(cache.type);
	const vec3 logical_position = to_logical_local_position(bounds, vec3(base_vertex));

	if (cache.mode == PrimitiveTransformMode::CubeX) {
		if (logical_position.x > 0.5f) {
			*transform_out = &cache.secondary_transform;
			*normal_transform_out = &cache.secondary_normal_transform;
		}
	} else if (cache.mode == PrimitiveTransformMode::CubeY) {
		if (logical_position.y > 0.5f) {
			*transform_out = &cache.secondary_transform;
			*normal_transform_out = &cache.secondary_normal_transform;
		}
	} else if (cache.mode == PrimitiveTransformMode::CubeZ) {
		if (logical_position.z > 0.5f) {
			*transform_out = &cache.secondary_transform;
			*normal_transform_out = &cache.secondary_normal_transform;
		}
	} else if (logical_position.z > 0.0f) {
		*transform_out = &cache.secondary_transform;
		*normal_transform_out = &cache.secondary_normal_transform;
	}
}

vec3 transform_world_vertex(const PrimitiveTransformCache &cache, const vec3 &base_position)
{
	const vec4 base_vertex(base_position, 1.0f);
	const vec4 local_vertex = transform_local_vertex(cache, base_vertex);
	const mat4 *transform = nullptr;
	const mat3 *normal_transform = nullptr;
	select_world_transform(cache, base_vertex, &transform, &normal_transform);
	(void)normal_transform;
	return vec3((*transform) * local_vertex);
}

vec3 orthogonal_unit_vector(const vec3 &normal)
{
	const vec3 reference =
		std::fabs(normal.y) < 0.85f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
	const vec3 tangent = glm::cross(normal, reference);
	const float tangent_length2 = glm::dot(tangent, tangent);
	if (tangent_length2 <= (kMinDelta * kMinDelta)) {
		return vec3(1.0f, 0.0f, 0.0f);
	}
	return tangent / std::sqrt(tangent_length2);
}

struct FaceProjectionBasis {
	vec3 tangent{1.0f, 0.0f, 0.0f};
	vec3 bitangent{0.0f, 1.0f, 0.0f};
};

FaceProjectionBasis face_projection_basis(const vec3 &normal)
{
	vec3 axis = normal;
	if (glm::dot(axis, axis) <= (kMinDelta * kMinDelta)) {
		axis = vec3(0.0f, 1.0f, 0.0f);
	} else {
		axis = glm::normalize(axis);
	}

	const vec3 abs_axis = glm::abs(axis);
	if (abs_axis.x >= abs_axis.y && abs_axis.x >= abs_axis.z) {
		return axis.x >= 0.0f
		           ? FaceProjectionBasis{vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)}
		           : FaceProjectionBasis{vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f)};
	}
	if (abs_axis.y >= abs_axis.z) {
		return axis.y >= 0.0f
		           ? FaceProjectionBasis{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)}
		           : FaceProjectionBasis{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)};
	}
	return axis.z >= 0.0f
	           ? FaceProjectionBasis{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)}
	           : FaceProjectionBasis{vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)};
}

float projected_axis_scale(const mat4 &transform, const vec3 &axis)
{
	const vec3 transformed_axis = mat3(transform) * axis;
	const float axis_length = glm::length(transformed_axis);
	return axis_length <= kMinDelta ? 1.0f : axis_length;
}

float effective_texture_tile_scale(float texscale)
{
	constexpr float kLegacyDefaultTextureScale = 0.125f;
	constexpr float kAutoTextureTileDensity = 0.90f;
	if (texscale <= (kLegacyDefaultTextureScale + 0.0005f)) {
		return kAutoTextureTileDensity;
	}
	return std::max(texscale, 0.0f) * 2.0f;
}

vec2 compute_face_aligned_uv(const vec3 &local_position,
                             const vec3 &local_normal,
                             const mat4 &world_transform,
                             float texscale)
{
	const FaceProjectionBasis basis = face_projection_basis(local_normal);
	const float tile_scale = effective_texture_tile_scale(texscale);
	return vec2(glm::dot(local_position, basis.tangent) *
	                projected_axis_scale(world_transform, basis.tangent),
	            glm::dot(local_position, basis.bitangent) *
	                projected_axis_scale(world_transform, basis.bitangent)) *
	       tile_scale;
}

void invalidate_collision_geometry(PrimitiveInstance *instance)
{
	if (instance == nullptr) {
		return;
	}
	instance->collision_geometry_dirty = true;
	instance->collision_geometry_cache = CollisionGeometry{};
}

vec3 clamp_vector_magnitude(const vec3 &value, float max_magnitude)
{
	if (max_magnitude <= 0.0f) {
		return vec3(0.0f);
	}
	const float magnitude2 = glm::dot(value, value);
	if (magnitude2 <= (max_magnitude * max_magnitude)) {
		return value;
	}
	const float magnitude = std::sqrt(magnitude2);
	if (magnitude <= kMinDelta) {
		return vec3(0.0f);
	}
	return value * (max_magnitude / magnitude);
}

void clamp_rotational_velocity(PrimitiveInstance *instance)
{
	if (instance == nullptr || instance->immovable) {
		return;
	}
	instance->rotational_velocity =
		clamp_vector_magnitude(instance->rotational_velocity, kMaxRotationalSpeed);
}

void append_unique_axis(std::vector<vec3> *axes, const vec3 &axis)
{
	if (axes == nullptr) {
		return;
	}

	const float axis_length = glm::length(axis);
	if (axis_length <= kMinDelta) {
		return;
	}

	const vec3 normalized_axis = axis / axis_length;
	for (const vec3 &existing_axis : *axes) {
		if (std::fabs(glm::dot(existing_axis, normalized_axis)) >= 0.999f) {
			return;
		}
	}
	axes->push_back(normalized_axis);
}

template <std::size_t N>
PrimitiveBounds bounds_for_vertices(const std::array<vec3, N> &vertices)
{
	PrimitiveBounds bounds;
	if (vertices.empty()) {
		return bounds;
	}

	bounds.min = vec3(std::numeric_limits<float>::max());
	bounds.max = vec3(std::numeric_limits<float>::lowest());
	for (const vec3 &vertex : vertices) {
		bounds.min = glm::min(bounds.min, vertex);
		bounds.max = glm::max(bounds.max, vertex);
	}
	bounds.center = (bounds.min + bounds.max) * 0.5f;
	bounds.half_extents = (bounds.max - bounds.min) * 0.5f;
	bounds.valid = true;
	return bounds;
}

CollisionTriangle build_collision_triangle(const vec3 &a, const vec3 &b, const vec3 &c)
{
	CollisionTriangle triangle;
	triangle.vertices = {a, b, c};
	triangle.bounds = bounds_for_vertices(triangle.vertices);
	triangle.centroid = (a + b + c) / 3.0f;
	const vec3 raw_normal = glm::cross(b - a, c - a);
	const float normal_length = glm::length(raw_normal);
	if (normal_length > kMinDelta) {
		triangle.normal = raw_normal / normal_length;
		triangle.valid = true;
	}
	return triangle;
}

vec3 center_of_mass_for_triangles(const std::vector<CollisionTriangle> &triangles, const vec3 &fallback)
{
	double signed_volume = 0.0;
	glm::dvec3 weighted_center(0.0);
	double total_area = 0.0;
	glm::dvec3 weighted_area_center(0.0);

	for (const CollisionTriangle &triangle : triangles) {
		if (!triangle.valid) {
			continue;
		}
		const glm::dvec3 a(triangle.vertices[0]);
		const glm::dvec3 b(triangle.vertices[1]);
		const glm::dvec3 c(triangle.vertices[2]);
		const glm::dvec3 cross = glm::cross(b, c);
		const double tetra_volume = glm::dot(a, cross) / 6.0;
		signed_volume += tetra_volume;
		weighted_center += (a + b + c) * (tetra_volume / 4.0);

		const double area = glm::length(glm::cross(b - a, c - a)) * 0.5;
		if (area > static_cast<double>(kMinDelta)) {
			total_area += area;
			weighted_area_center += ((a + b + c) / 3.0) * area;
		}
	}

	if (std::fabs(signed_volume) > static_cast<double>(kMinDelta)) {
		return vec3(weighted_center / signed_volume);
	}
	if (total_area > static_cast<double>(kMinDelta)) {
		return vec3(weighted_area_center / total_area);
	}
	return fallback;
}

CollisionObbData build_obb_data(const std::array<vec3, 8> &vertices)
{
	CollisionObbData obb;
	const vec3 edge_x = vertices[4] - vertices[6];
	const vec3 edge_y = vertices[7] - vertices[6];
	const vec3 edge_z = vertices[2] - vertices[6];

	const float edge_x_length = glm::length(edge_x);
	const float edge_y_length = glm::length(edge_y);
	const float edge_z_length = glm::length(edge_z);
	if (edge_x_length <= kMinDelta || edge_y_length <= kMinDelta || edge_z_length <= kMinDelta) {
		return obb;
	}

	vec3 axis_x = edge_x / edge_x_length;
	vec3 axis_y = edge_y - (glm::dot(edge_y, axis_x) * axis_x);
	const float axis_y_length = glm::length(axis_y);
	if (axis_y_length <= kMinDelta) {
		return obb;
	}
	axis_y /= axis_y_length;

	vec3 axis_z = glm::cross(axis_x, axis_y);
	const float axis_z_length = glm::length(axis_z);
	if (axis_z_length <= kMinDelta) {
		return obb;
	}
	axis_z /= axis_z_length;

	vec3 average_center(0.0f);
	for (const vec3 &vertex : vertices) {
		average_center += vertex;
	}
	average_center /= static_cast<float>(vertices.size());

	vec3 min_projection(std::numeric_limits<float>::max());
	vec3 max_projection(std::numeric_limits<float>::lowest());
	for (const vec3 &vertex : vertices) {
		const vec3 delta = vertex - average_center;
		const vec3 projection(glm::dot(delta, axis_x),
		                      glm::dot(delta, axis_y),
		                      glm::dot(delta, axis_z));
		min_projection = glm::min(min_projection, projection);
		max_projection = glm::max(max_projection, projection);
	}

	obb.axes = {axis_x, axis_y, axis_z};
	obb.half_extents = (max_projection - min_projection) * 0.5f;
	obb.center = average_center +
	             axis_x * ((min_projection.x + max_projection.x) * 0.5f) +
	             axis_y * ((min_projection.y + max_projection.y) * 0.5f) +
	             axis_z * ((min_projection.z + max_projection.z) * 0.5f);
	obb.valid = glm::all(glm::greaterThan(obb.half_extents, vec3(0.0f)));
	return obb;
}

template <typename VertexContainer>
float average_radial_distance_to_vertices(const VertexContainer &vertices, const vec3 &origin)
{
	if (vertices.empty()) {
		return 0.0f;
	}

	double total_distance = 0.0;
	for (const vec3 &vertex : vertices) {
		total_distance += glm::length(vertex - origin);
	}
	return static_cast<float>(total_distance / static_cast<double>(vertices.size()));
}

float distance_rejection_metric(float squared_distance)
{
	const float delta = squared_distance - 1.0f;
	const float taylor_distance =
		(0.5f * delta) - (0.125f * delta * delta);
	if (squared_distance <= 3.0f) {
		return taylor_distance;
	}

	// The quadratic requested by the user is only a local approximation around s=1.
	// Fall back to a monotonic distance term for larger separations so far pairs still reject.
	return std::sqrt(std::max(squared_distance, 0.0f)) - 1.0f;
}

bool passes_distance_rejection_test(const CollisionGeometry &a, const CollisionGeometry &b)
{
	if (!a.valid || !b.valid) {
		return true;
	}

	const float combined_average_radius =
		a.average_radial_distance_to_vertex + b.average_radial_distance_to_vertex;
	if (combined_average_radius <= kMinDelta) {
		return true;
	}

	const vec3 delta = b.center_of_mass - a.center_of_mass;
	const float s = glm::dot(delta, delta);
	return distance_rejection_metric(s) <
	       (kCollisionDistanceRejectionFactor * combined_average_radius);
}

std::array<vec3, 8> vertices_for_bounds(const vec3 &min_corner, const vec3 &max_corner)
{
	return {
		vec3(max_corner.x, min_corner.y, max_corner.z),
		vec3(max_corner.x, max_corner.y, max_corner.z),
		vec3(min_corner.x, min_corner.y, max_corner.z),
		vec3(min_corner.x, max_corner.y, max_corner.z),
		vec3(max_corner.x, min_corner.y, min_corner.z),
		vec3(max_corner.x, max_corner.y, min_corner.z),
		vec3(min_corner.x, min_corner.y, min_corner.z),
		vec3(min_corner.x, max_corner.y, min_corner.z)};
}

CollisionGeometry build_box_collision_geometry(const std::array<vec3, 8> &vertices)
{
	static const std::array<glm::ivec3, 12> kCollisionTriangles = {
		glm::ivec3(0, 3, 2), glm::ivec3(0, 1, 3),
		glm::ivec3(5, 4, 7), glm::ivec3(4, 6, 7),
		glm::ivec3(4, 0, 6), glm::ivec3(0, 2, 6),
		glm::ivec3(5, 3, 1), glm::ivec3(5, 7, 3),
		glm::ivec3(4, 1, 0), glm::ivec3(5, 1, 4),
		glm::ivec3(2, 3, 6), glm::ivec3(6, 3, 7)};

	CollisionGeometry geometry;
	geometry.kind = CollisionGeometryKind::BoxHull;
	geometry.convex_vertices.assign(vertices.begin(), vertices.end());
	geometry.face_axes.reserve(3);
	geometry.edge_directions.reserve(3);
	geometry.bounds.min = vec3(std::numeric_limits<float>::max());
	geometry.bounds.max = vec3(std::numeric_limits<float>::lowest());

	for (std::size_t vertex_index = 0; vertex_index < vertices.size(); ++vertex_index) {
		const vec3 &vertex = vertices[vertex_index];
		geometry.vertices[vertex_index] = vertex;
		geometry.centroid += vertex;
		geometry.bounds.min = glm::min(geometry.bounds.min, vertex);
		geometry.bounds.max = glm::max(geometry.bounds.max, vertex);
	}

	geometry.centroid /= static_cast<float>(vertices.size());
	geometry.bounds.center = geometry.centroid;
	geometry.bounds.half_extents = (geometry.bounds.max - geometry.bounds.min) * 0.5f;
	geometry.bounds.valid = true;
	geometry.triangles.reserve(kCollisionTriangles.size());

	for (const glm::ivec3 &triangle : kCollisionTriangles) {
		const vec3 &a = geometry.vertices[static_cast<std::size_t>(triangle.x)];
		const vec3 &b = geometry.vertices[static_cast<std::size_t>(triangle.y)];
		const vec3 &c = geometry.vertices[static_cast<std::size_t>(triangle.z)];
		geometry.triangles.push_back(build_collision_triangle(a, b, c));
	}

	geometry.obb = build_obb_data(geometry.vertices);
	if (geometry.obb.valid) {
		for (const vec3 &axis : geometry.obb.axes) {
			geometry.face_axes.push_back(axis);
			geometry.edge_directions.push_back(axis);
		}
	}
	geometry.center_of_mass = geometry.obb.valid ? geometry.obb.center : geometry.bounds.center;
	geometry.average_radial_distance_to_vertex =
		average_radial_distance_to_vertices(geometry.vertices, geometry.center_of_mass);
	geometry.valid = true;
	return geometry;
}

CollisionGeometry build_collision_geometry(const PrimitiveInstance &instance)
{
	static const std::array<vec3, 8> kBaseCollisionVertices = {
		vec3(0.5f, 0.0f, 0.5f),
		vec3(0.5f, 1.0f, 0.5f),
		vec3(-0.5f, 0.0f, 0.5f),
		vec3(-0.5f, 1.0f, 0.5f),
		vec3(0.5f, 0.0f, -0.5f),
		vec3(0.5f, 1.0f, -0.5f),
		vec3(-0.5f, 0.0f, -0.5f),
		vec3(-0.5f, 1.0f, -0.5f)};

	const PrimitiveTransformCache cache = build_transform_cache(instance, false);
	if (instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh()) {
		const Mesh &mesh = instance.resolved_geometry->triangleMesh();
		if (mesh.faces.empty()) {
			return {};
		}

		CollisionGeometry geometry;
		geometry.kind = instance.resolved_geometry->collisionPolicy() ==
		                        PrimitiveCollisionPolicy::ConvexMesh
			? CollisionGeometryKind::ConvexHull
			: CollisionGeometryKind::TriangleMesh;
		geometry.source_mesh = &mesh;
		geometry.source_transform = cache.primary_transform;
		geometry.triangles.assign(mesh.faces.size(), {});
		geometry.bounds.min = vec3(std::numeric_limits<float>::max());
		geometry.bounds.max = vec3(std::numeric_limits<float>::lowest());
		std::vector<vec3> transformed_vertices;
		transformed_vertices.reserve(mesh.vertices.size());
		bool has_valid_triangle = false;

		for (const vec3 &vertex : mesh.vertices) {
			transformed_vertices.push_back(
				vec3(cache.primary_transform * vec4(vertex, 1.0f)));
		}
		if (geometry.kind == CollisionGeometryKind::ConvexHull) {
			geometry.convex_vertices = transformed_vertices;
		}

		for (std::size_t face_index = 0; face_index < mesh.faces.size(); ++face_index) {
			const glm::ivec3 &face = mesh.faces[face_index];
			const vec3 &a = transformed_vertices[static_cast<std::size_t>(face.x)];
			const vec3 &b = transformed_vertices[static_cast<std::size_t>(face.y)];
			const vec3 &c = transformed_vertices[static_cast<std::size_t>(face.z)];
			CollisionTriangle triangle = build_collision_triangle(a, b, c);
			geometry.triangles[face_index] = triangle;
			if (!triangle.valid) {
				continue;
			}
			has_valid_triangle = true;
			geometry.bounds.min = glm::min(geometry.bounds.min, triangle.bounds.min);
			geometry.bounds.max = glm::max(geometry.bounds.max, triangle.bounds.max);
		}
		if (!has_valid_triangle) {
			return {};
		}

		geometry.bounds.center = (geometry.bounds.min + geometry.bounds.max) * 0.5f;
		geometry.bounds.half_extents = (geometry.bounds.max - geometry.bounds.min) * 0.5f;
		geometry.bounds.valid = true;
		geometry.centroid = geometry.bounds.center;
		geometry.center_of_mass =
			center_of_mass_for_triangles(geometry.triangles, geometry.bounds.center);
		geometry.average_radial_distance_to_vertex =
			average_radial_distance_to_vertices(transformed_vertices, geometry.center_of_mass);
		geometry.valid = true;
		return geometry;
	}

	std::array<vec3, 8> world_vertices{};
	for (std::size_t vertex_index = 0; vertex_index < kBaseCollisionVertices.size(); ++vertex_index) {
		world_vertices[vertex_index] =
			transform_world_vertex(cache, kBaseCollisionVertices[vertex_index]);
	}
	return build_box_collision_geometry(world_vertices);
}

const CollisionGeometry &collision_geometry_for_instance(const PrimitiveInstance &instance)
{
	if (!instance.collision_geometry_dirty) {
		return instance.collision_geometry_cache;
	}

	instance.collision_geometry_cache = build_collision_geometry(instance);
	instance.collision_geometry_dirty = false;
	return instance.collision_geometry_cache;
}

mat4 build_rotation_delta(const vec3 &delta_degrees)
{
	mat4 rotation(1.0f);
	if (std::fabs(delta_degrees.x) > kMinDelta) {
		rotation = glm::rotate(rotation, glm::radians(delta_degrees.x), vec3(1.0f, 0.0f, 0.0f));
	}
	if (std::fabs(delta_degrees.y) > kMinDelta) {
		rotation = glm::rotate(rotation, glm::radians(delta_degrees.y), vec3(0.0f, 1.0f, 0.0f));
	}
	if (std::fabs(delta_degrees.z) > kMinDelta) {
		rotation = glm::rotate(rotation, glm::radians(delta_degrees.z), vec3(0.0f, 0.0f, 1.0f));
	}
	return rotation;
}

void apply_world_translation(PrimitiveInstance *instance, const vec3 &delta)
{
	if (instance == nullptr || glm::dot(delta, delta) <= (kMinDelta * kMinDelta)) {
		return;
	}

	const mat4 translation = glm::translate(mat4(1.0f), delta);
	instance->primary_transform = translation * instance->primary_transform;
	instance->secondary_transform = translation * instance->secondary_transform;
	instance->position = glm::vec3(instance->primary_transform[3]);
	invalidate_collision_geometry(instance);
}

void apply_world_rotation(PrimitiveInstance *instance, const vec3 &delta_degrees, const vec3 &pivot)
{
	if (instance == nullptr ||
	    glm::dot(delta_degrees, delta_degrees) <= (kMinDelta * kMinDelta)) {
		return;
	}

	const mat4 rotation = build_rotation_delta(delta_degrees);
	const mat4 around_pivot =
		glm::translate(mat4(1.0f), pivot) *
		rotation *
		glm::translate(mat4(1.0f), -pivot);
	instance->primary_transform = around_pivot * instance->primary_transform;
	instance->secondary_transform = around_pivot * instance->secondary_transform;
	instance->position = glm::vec3(instance->primary_transform[3]);
	instance->rotation_degrees = transform_euler_degrees(instance->primary_transform);
	invalidate_collision_geometry(instance);
}

void capture_previous_render_state(PrimitiveInstance *instance)
{
	if (instance == nullptr || instance->removed || !uses_dynamic_cube_preview_path(*instance)) {
		return;
	}

	const PrimitiveBounds bounds = collision_geometry_for_instance(*instance).bounds;
	instance->previous_primary_transform = instance->primary_transform;
	instance->previous_secondary_transform = instance->secondary_transform;
	instance->previous_bounds_center = bounds.valid ? bounds.center : instance->position;
	instance->has_previous_render_state = true;
}

void wake_instance(PrimitiveInstance *instance)
{
	if (instance == nullptr || instance->removed || instance->immovable || !instance->simulation_active) {
		return;
	}
	instance->sleeping = false;
	instance->sleep_timer = 0.0f;
}

void put_instance_to_sleep(PrimitiveInstance *instance)
{
	if (instance == nullptr || instance->removed || instance->immovable || !instance->simulation_active) {
		return;
	}
	instance->sleeping = true;
	instance->sleep_timer = 0.0f;
	instance->velocity = vec3(0.0f);
	instance->rotational_velocity = vec3(0.0f);
}

bool collision_contact_requires_wake(float closing_speed, float penetration)
{
	return closing_speed >= kWakeLinearSpeed || penetration > kCollisionWakePenetration;
}

float collision_restitution_for_closing_speed(float closing_speed)
{
	return closing_speed < kRestingContactImpactSpeed ? 0.0f : kCollisionRestitution;
}

float compute_broad_phase_cell_size(const std::vector<std::size_t> &collidable_indices,
                                    const std::vector<const CollisionGeometry *> &collision_geometries)
{
	double total_extent = 0.0;
	std::size_t valid_count = 0;
	for (std::size_t index : collidable_indices) {
		if (index >= collision_geometries.size() || collision_geometries[index] == nullptr) {
			continue;
		}
		const float extent = bounds_extent_metric(collision_geometries[index]->bounds);
		if (extent <= kMinDelta) {
			continue;
		}
		total_extent += static_cast<double>(extent);
		++valid_count;
	}
	if (valid_count == 0) {
		return 1.0f;
	}
	const float average_extent = static_cast<float>(total_extent / static_cast<double>(valid_count));
	return std::clamp(average_extent * kBroadPhaseCellScale,
	                  kBroadPhaseCellSizeMin,
	                  kBroadPhaseCellSizeMax);
}

int broad_phase_cell_coordinate(float value, float cell_size)
{
	return static_cast<int>(std::floor(value / cell_size));
}

std::vector<BroadPhasePair> build_broad_phase_pairs(const std::vector<PrimitiveInstance> &instances,
                                                    const std::vector<std::size_t> &collidable_indices,
                                                    const std::vector<const CollisionGeometry *> &collision_geometries)
{
	std::vector<BroadPhasePair> pairs;
	if (collidable_indices.size() < 2) {
		return pairs;
	}

	const float cell_size = compute_broad_phase_cell_size(collidable_indices, collision_geometries);
	std::unordered_map<SpatialHashCell, std::vector<std::size_t>, SpatialHashCellHasher> buckets;
	buckets.reserve(collidable_indices.size() * 2u);
	std::unordered_set<BroadPhasePairKey, BroadPhasePairKeyHasher> seen_pairs;
	seen_pairs.reserve(collidable_indices.size() * 4u);

	for (std::size_t index : collidable_indices) {
		if (index >= instances.size() ||
		    index >= collision_geometries.size() ||
		    collision_geometries[index] == nullptr ||
		    !is_collision_candidate(instances[index])) {
			continue;
		}
		const PrimitiveBounds &bounds = collision_geometries[index]->bounds;
		if (!bounds.valid) {
			continue;
		}

		const int min_x = broad_phase_cell_coordinate(bounds.min.x, cell_size);
		const int min_y = broad_phase_cell_coordinate(bounds.min.y, cell_size);
		const int min_z = broad_phase_cell_coordinate(bounds.min.z, cell_size);
		const int max_x = broad_phase_cell_coordinate(bounds.max.x, cell_size);
		const int max_y = broad_phase_cell_coordinate(bounds.max.y, cell_size);
		const int max_z = broad_phase_cell_coordinate(bounds.max.z, cell_size);

		for (int x = min_x; x <= max_x; ++x) {
			for (int y = min_y; y <= max_y; ++y) {
				for (int z = min_z; z <= max_z; ++z) {
					const SpatialHashCell cell{x, y, z};
						std::vector<std::size_t> &occupants = buckets[cell];
						for (std::size_t other_index : occupants) {
							if (other_index >= instances.size() ||
							    other_index >= collision_geometries.size() ||
							    collision_geometries[other_index] == nullptr ||
							    !is_collision_candidate(instances[other_index])) {
								continue;
							}
						if (!is_dynamic_simulation_active(instances[index]) &&
						    !is_dynamic_simulation_active(instances[other_index])) {
							continue;
						}
						const BroadPhasePairKey key{
							std::min(index, other_index),
							std::max(index, other_index)};
						if (seen_pairs.insert(key).second) {
							pairs.push_back({key.a, key.b});
						}
					}
					occupants.push_back(index);
				}
			}
		}
	}

	return pairs;
}

bool compute_aabb_overlap(const PrimitiveBounds &a,
                          const PrimitiveBounds &b,
                          vec3 *normal,
                          float *penetration,
                          vec3 *contact_point,
                          const vec3 *preferred_direction = nullptr)
{
	const vec3 overlap = glm::min(a.max, b.max) - glm::max(a.min, b.min);
	if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) {
		return false;
	}

	int axis = -1;
	float strongest_closing_speed = 0.0f;
	const vec3 center_delta = b.center - a.center;
	if (preferred_direction != nullptr) {
		for (int candidate_axis = 0; candidate_axis < 3; ++candidate_axis) {
			const float axis_speed = (*preferred_direction)[candidate_axis];
			if (std::fabs(axis_speed) <= kMinDelta) {
				continue;
			}
			const float axis_center_delta = center_delta[candidate_axis];
			const bool closing =
				std::fabs(axis_center_delta) <= kMinDelta ||
				(axis_speed < 0.0f && axis_center_delta >= 0.0f) ||
				(axis_speed > 0.0f && axis_center_delta <= 0.0f);
			if (!closing) {
				continue;
			}
			const float speed_magnitude = std::fabs(axis_speed);
			if (axis < 0 || speed_magnitude > strongest_closing_speed) {
				axis = candidate_axis;
				strongest_closing_speed = speed_magnitude;
			}
		}
	}

	if (axis < 0) {
		axis = 0;
		if (overlap.y < overlap.x) {
			axis = 1;
		}
		if (overlap.z < overlap[axis]) {
			axis = 2;
		}
	}

	vec3 axis_normal(0.0f);
	float sign = 1.0f;
	if (std::fabs(center_delta[axis]) > kMinDelta) {
		sign = center_delta[axis] >= 0.0f ? 1.0f : -1.0f;
	} else if (preferred_direction != nullptr && std::fabs((*preferred_direction)[axis]) > kMinDelta) {
		sign = (*preferred_direction)[axis] < 0.0f ? 1.0f : -1.0f;
	}
	axis_normal[axis] = sign;

	if (normal != nullptr) {
		*normal = axis_normal;
	}
	if (penetration != nullptr) {
		*penetration = overlap[axis];
	}
	if (contact_point != nullptr) {
		*contact_point = (glm::max(a.min, b.min) + glm::min(a.max, b.max)) * 0.5f;
	}
	return true;
}

template <std::size_t N>
void project_vertices_onto_axis(const std::array<vec3, N> &vertices,
                                const vec3 &axis,
                                float *min_projection,
                                float *max_projection)
{
	if (min_projection == nullptr || max_projection == nullptr) {
		return;
	}

	float minimum = glm::dot(vertices[0], axis);
	float maximum = minimum;
	for (std::size_t index = 1; index < vertices.size(); ++index) {
		const float projection = glm::dot(vertices[index], axis);
		minimum = std::min(minimum, projection);
		maximum = std::max(maximum, projection);
	}

	*min_projection = minimum;
	*max_projection = maximum;
}

template <std::size_t N>
vec3 support_vertex(const std::array<vec3, N> &vertices, const vec3 &axis)
{
	float best_projection = glm::dot(vertices[0], axis);
	vec3 best_vertex = vertices[0];
	for (std::size_t index = 1; index < vertices.size(); ++index) {
		const float projection = glm::dot(vertices[index], axis);
		if (projection > best_projection) {
			best_projection = projection;
			best_vertex = vertices[index];
		}
	}
	return best_vertex;
}

template <std::size_t N>
vec3 support_feature_center(const std::array<vec3, N> &vertices, const vec3 &axis)
{
	constexpr float kSupportProjectionTolerance = 0.0001f;
	float best_projection = glm::dot(vertices[0], axis);
	for (std::size_t index = 1; index < vertices.size(); ++index) {
		best_projection = std::max(best_projection, glm::dot(vertices[index], axis));
	}

	vec3 accumulated_center(0.0f);
	std::size_t support_vertex_count = 0;
	for (const vec3 &vertex : vertices) {
		if (best_projection - glm::dot(vertex, axis) > kSupportProjectionTolerance) {
			continue;
		}
		accumulated_center += vertex;
		++support_vertex_count;
	}
	return support_vertex_count == 0
		? support_vertex(vertices, axis)
		: accumulated_center / static_cast<float>(support_vertex_count);
}

bool compute_triangle_triangle_overlap(const CollisionTriangle &a,
                                       const CollisionTriangle &b,
                                       vec3 *normal,
                                       float *penetration,
                                       vec3 *contact_point);

vec3 support_feature_center_for_obb(const CollisionObbData &obb, const vec3 &axis)
{
	constexpr float kSupportAxisTolerance = 0.0001f;
	vec3 center = obb.center;
	for (int axis_index = 0; axis_index < 3; ++axis_index) {
		const vec3 &obb_axis = obb.axes[static_cast<std::size_t>(axis_index)];
		const float projection = glm::dot(axis, obb_axis);
		if (std::fabs(projection) <= kSupportAxisTolerance) {
			continue;
		}
		center += obb_axis * obb.half_extents[static_cast<std::size_t>(axis_index)] *
		          (projection >= 0.0f ? 1.0f : -1.0f);
	}
	return center;
}

bool compute_obb_overlap(const CollisionGeometry &a,
                         const CollisionGeometry &b,
                         vec3 *normal,
                         float *penetration,
                         vec3 *contact_point)
{
	if (!a.obb.valid || !b.obb.valid) {
		return false;
	}

	constexpr float kAxisEpsilon = 0.00001f;
	float rotation[3][3];
	float abs_rotation[3][3];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			rotation[i][j] = glm::dot(a.obb.axes[static_cast<std::size_t>(i)],
			                          b.obb.axes[static_cast<std::size_t>(j)]);
			abs_rotation[i][j] = std::fabs(rotation[i][j]) + kAxisEpsilon;
		}
	}

	const vec3 center_delta = b.obb.center - a.obb.center;
	float translated[3] = {
		glm::dot(center_delta, a.obb.axes[0]),
		glm::dot(center_delta, a.obb.axes[1]),
		glm::dot(center_delta, a.obb.axes[2])};

	float minimum_overlap = std::numeric_limits<float>::max();
	vec3 minimum_axis(0.0f);

	auto consider_axis = [&](const vec3 &axis, float axis_distance, float axis_radius) -> bool {
		if (axis_radius <= kMinDelta) {
			return true;
		}
		const float separation = std::fabs(axis_distance) - axis_radius;
		if (separation > 0.0f) {
			return false;
		}
		const float overlap = axis_radius - std::fabs(axis_distance);
		vec3 oriented_axis = axis;
		if (glm::dot(center_delta, oriented_axis) < 0.0f) {
			oriented_axis = -oriented_axis;
		}
		if (overlap < minimum_overlap) {
			minimum_overlap = overlap;
			minimum_axis = oriented_axis;
		}
		return true;
	};

	for (int i = 0; i < 3; ++i) {
		const float radius_a = a.obb.half_extents[static_cast<std::size_t>(i)];
		const float radius_b =
			b.obb.half_extents.x * abs_rotation[i][0] +
			b.obb.half_extents.y * abs_rotation[i][1] +
			b.obb.half_extents.z * abs_rotation[i][2];
		if (!consider_axis(a.obb.axes[static_cast<std::size_t>(i)], translated[i], radius_a + radius_b)) {
			return false;
		}
	}

	for (int j = 0; j < 3; ++j) {
		const float distance =
			glm::dot(center_delta, b.obb.axes[static_cast<std::size_t>(j)]);
		const float radius_a =
			a.obb.half_extents.x * abs_rotation[0][j] +
			a.obb.half_extents.y * abs_rotation[1][j] +
			a.obb.half_extents.z * abs_rotation[2][j];
		const float radius_b = b.obb.half_extents[static_cast<std::size_t>(j)];
		if (!consider_axis(b.obb.axes[static_cast<std::size_t>(j)], distance, radius_a + radius_b)) {
			return false;
		}
	}

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			const vec3 axis = glm::cross(a.obb.axes[static_cast<std::size_t>(i)],
			                             b.obb.axes[static_cast<std::size_t>(j)]);
			const float axis_length = glm::length(axis);
			if (axis_length <= kMinDelta) {
				continue;
			}
			const vec3 normalized_axis = axis / axis_length;
			const int i1 = (i + 1) % 3;
			const int i2 = (i + 2) % 3;
			const int j1 = (j + 1) % 3;
			const int j2 = (j + 2) % 3;
			const float distance =
				std::fabs(translated[i2] * rotation[i1][j] - translated[i1] * rotation[i2][j]);
			const float radius_a =
				a.obb.half_extents[static_cast<std::size_t>(i1)] * abs_rotation[i2][j] +
				a.obb.half_extents[static_cast<std::size_t>(i2)] * abs_rotation[i1][j];
			const float radius_b =
				b.obb.half_extents[static_cast<std::size_t>(j1)] * abs_rotation[i][j2] +
				b.obb.half_extents[static_cast<std::size_t>(j2)] * abs_rotation[i][j1];
			if (!consider_axis(normalized_axis, distance, radius_a + radius_b)) {
				return false;
			}
		}
	}

	if (normal != nullptr) {
		*normal = minimum_axis;
	}
	if (penetration != nullptr) {
		*penetration = minimum_overlap;
	}
	if (contact_point != nullptr) {
		const vec3 point_a = support_feature_center_for_obb(a.obb, minimum_axis);
		const vec3 point_b = support_feature_center_for_obb(b.obb, -minimum_axis);
		*contact_point = (point_a + point_b) * 0.5f;
	}
	return true;
}

bool is_default_uniform_sphere(const PrimitiveInstance &instance,
	                           const CollisionGeometry &geometry)
{
	const bool is_legacy_default_sphere = instance.type == "Sphere";
	const bool is_procedural_default_sphere =
		instance.shape_specification &&
		instance.shape_specification->family() == ShapeFamily::Sphere &&
		instance.shape_specification->isDefaultFamilyShape();
	if ((!is_legacy_default_sphere && !is_procedural_default_sphere) ||
	    !geometry.bounds.valid) {
		return false;
	}
	const vec3 half_extents = geometry.bounds.half_extents;
	const float largest_extent = std::max(half_extents.x, std::max(half_extents.y, half_extents.z));
	if (largest_extent <= kMinDelta) {
		return false;
	}
	const float uniformity_tolerance = largest_extent * 0.001f;
	return std::fabs(half_extents.x - half_extents.y) <= uniformity_tolerance &&
	       std::fabs(half_extents.x - half_extents.z) <= uniformity_tolerance;
}

bool is_default_uniform_cylinder(const PrimitiveInstance &instance,
	                             const CollisionGeometry &geometry)
{
	const bool is_legacy_default_cylinder = instance.type == "Cylinder";
	const bool is_procedural_default_cylinder =
		instance.shape_specification &&
		instance.shape_specification->family() == ShapeFamily::Cylinder &&
		instance.shape_specification->isDefaultFamilyShape();
	if ((!is_legacy_default_cylinder && !is_procedural_default_cylinder) ||
	    !geometry.bounds.valid || geometry.source_mesh == nullptr) {
		return false;
	}

	const float radial_x = glm::length(vec3(geometry.source_transform[0])) * 0.5f;
	const float radial_z = glm::length(vec3(geometry.source_transform[2])) * 0.5f;
	const float largest_radius = std::max(radial_x, radial_z);
	return largest_radius > kMinDelta &&
	       std::fabs(radial_x - radial_z) <= largest_radius * 0.001f;
}

float uniform_sphere_radius(const CollisionGeometry &geometry)
{
	return (geometry.bounds.half_extents.x +
	        geometry.bounds.half_extents.y +
	        geometry.bounds.half_extents.z) /
	       3.0f;
}

bool compute_sphere_sphere_overlap(const CollisionGeometry &a,
	                               const CollisionGeometry &b,
	                               const vec3 &relative_velocity,
	                               vec3 *normal,
	                               float *penetration,
	                               vec3 *contact_point)
{
	const float radius_a = uniform_sphere_radius(a);
	const float radius_b = uniform_sphere_radius(b);
	const vec3 center_delta = b.bounds.center - a.bounds.center;
	const float center_distance = glm::length(center_delta);
	const float combined_radius = radius_a + radius_b;
	if (center_distance >= combined_radius) {
		return false;
	}

	vec3 collision_normal(1.0f, 0.0f, 0.0f);
	if (center_distance > kMinDelta) {
		collision_normal = center_delta / center_distance;
	} else if (glm::length(relative_velocity) > kMinDelta) {
		collision_normal = -glm::normalize(relative_velocity);
	}
	if (normal != nullptr) {
		*normal = collision_normal;
	}
	if (penetration != nullptr) {
		*penetration = combined_radius - center_distance;
	}
	if (contact_point != nullptr) {
		const vec3 point_a = a.bounds.center + collision_normal * radius_a;
		const vec3 point_b = b.bounds.center - collision_normal * radius_b;
		*contact_point = (point_a + point_b) * 0.5f;
	}
	return true;
}

bool compute_sphere_box_overlap(const CollisionGeometry &sphere,
	                            const CollisionGeometry &box,
	                            bool sphere_is_first,
	                            vec3 *normal,
	                            float *penetration,
	                            vec3 *contact_point)
{
	if (!box.obb.valid) {
		return false;
	}
	const float radius = uniform_sphere_radius(sphere);
	const vec3 center_offset = sphere.bounds.center - box.obb.center;
	vec3 closest_point = box.obb.center;
	bool sphere_center_inside_box = true;
	for (int axis_index = 0; axis_index < 3; ++axis_index) {
		const vec3 &box_axis = box.obb.axes[static_cast<std::size_t>(axis_index)];
		const float local_position = glm::dot(center_offset, box_axis);
		const float half_extent = box.obb.half_extents[static_cast<std::size_t>(axis_index)];
		const float clamped_position = glm::clamp(local_position, -half_extent, half_extent);
		closest_point += box_axis * clamped_position;
		sphere_center_inside_box = sphere_center_inside_box &&
		                           std::fabs(local_position) <= half_extent;
	}

	vec3 box_to_sphere_normal(0.0f);
	float overlap = 0.0f;
	if (!sphere_center_inside_box) {
		const vec3 separation = sphere.bounds.center - closest_point;
		const float distance = glm::length(separation);
		if (distance >= radius || distance <= kMinDelta) {
			return false;
		}
		box_to_sphere_normal = separation / distance;
		overlap = radius - distance;
	} else {
		float nearest_face_distance = std::numeric_limits<float>::max();
		for (int axis_index = 0; axis_index < 3; ++axis_index) {
			const vec3 &box_axis = box.obb.axes[static_cast<std::size_t>(axis_index)];
			const float local_position = glm::dot(center_offset, box_axis);
			const float half_extent = box.obb.half_extents[static_cast<std::size_t>(axis_index)];
			const float face_distance = half_extent - std::fabs(local_position);
			if (face_distance < nearest_face_distance) {
				nearest_face_distance = face_distance;
				box_to_sphere_normal = box_axis * (local_position >= 0.0f ? 1.0f : -1.0f);
			}
		}
		closest_point = sphere.bounds.center + box_to_sphere_normal * nearest_face_distance;
		overlap = radius + nearest_face_distance;
	}

	const vec3 collision_normal = sphere_is_first
		? -box_to_sphere_normal
		: box_to_sphere_normal;
	if (normal != nullptr) {
		*normal = collision_normal;
	}
	if (penetration != nullptr) {
		*penetration = overlap;
	}
	if (contact_point != nullptr) {
		const vec3 sphere_surface = sphere.bounds.center - box_to_sphere_normal * radius;
		*contact_point = (sphere_surface + closest_point) * 0.5f;
	}
	return true;
}

bool stabilize_cylinder_box_cap_contact(const CollisionGeometry &cylinder,
	                                    const CollisionGeometry &box,
	                                    bool cylinder_is_first,
	                                    vec3 *normal,
	                                    float *penetration,
	                                    vec3 *contact_point)
{
	if (!box.obb.valid) {
		return false;
	}
	const vec3 transformed_axis = vec3(cylinder.source_transform[1]);
	const float cylinder_height = glm::length(transformed_axis);
	if (cylinder_height <= kMinDelta) {
		return false;
	}
	const vec3 cylinder_axis = transformed_axis / cylinder_height;
	const float cylinder_half_height = cylinder_height * 0.5f;
	const float cylinder_radius =
		(glm::length(vec3(cylinder.source_transform[0])) +
		 glm::length(vec3(cylinder.source_transform[2]))) *
		0.25f;
	const vec3 center_delta = cylinder.bounds.center - box.obb.center;

	float minimum_overlap = std::numeric_limits<float>::max();
	int minimum_axis_index = -1;
	for (int axis_index = 0; axis_index < 3; ++axis_index) {
		const vec3 &box_axis = box.obb.axes[static_cast<std::size_t>(axis_index)];
		const float axial_alignment = std::fabs(glm::dot(box_axis, cylinder_axis));
		const float projected_radius =
			cylinder_half_height * axial_alignment +
			cylinder_radius * std::sqrt(std::max(0.0f, 1.0f - axial_alignment * axial_alignment));
		const float center_distance = std::fabs(glm::dot(center_delta, box_axis));
		const float axis_overlap =
			box.obb.half_extents[static_cast<std::size_t>(axis_index)] +
			projected_radius - center_distance;
		if (axis_overlap <= 0.0f) {
			return false;
		}
		if (axis_overlap < minimum_overlap) {
			minimum_overlap = axis_overlap;
			minimum_axis_index = axis_index;
		}
	}
	if (minimum_axis_index < 0) {
		return false;
	}

	const vec3 &support_axis = box.obb.axes[static_cast<std::size_t>(minimum_axis_index)];
	const float axis_alignment = glm::dot(support_axis, cylinder_axis);
	if (std::fabs(axis_alignment) < 0.999f) {
		return false;
	}
	const float signed_center_distance = glm::dot(center_delta, support_axis);
	const vec3 box_to_cylinder_normal =
		support_axis * (signed_center_distance >= 0.0f ? 1.0f : -1.0f);
	const vec3 collision_normal = cylinder_is_first
		? -box_to_cylinder_normal
		: box_to_cylinder_normal;

	if (normal != nullptr) {
		*normal = collision_normal;
	}
	if (penetration != nullptr) {
		*penetration = minimum_overlap;
	}
	if (contact_point != nullptr) {
		vec3 box_face_point = box.obb.center +
			box_to_cylinder_normal *
			box.obb.half_extents[static_cast<std::size_t>(minimum_axis_index)];
		for (int axis_index = 0; axis_index < 3; ++axis_index) {
			if (axis_index == minimum_axis_index) {
				continue;
			}
			const vec3 &box_axis = box.obb.axes[static_cast<std::size_t>(axis_index)];
			const float projected_center = glm::dot(center_delta, box_axis);
			const float clamped_center = glm::clamp(
				projected_center,
				-box.obb.half_extents[static_cast<std::size_t>(axis_index)],
				box.obb.half_extents[static_cast<std::size_t>(axis_index)]);
			box_face_point += box_axis * clamped_center;
		}
		const vec3 cylinder_cap_center =
			cylinder.bounds.center - box_to_cylinder_normal * cylinder_half_height;
		*contact_point = (box_face_point + cylinder_cap_center) * 0.5f;
	}
	return true;
}

PrimitiveBounds transform_mesh_bounds(const Mesh::CollisionBounds &local_bounds, const mat4 &transform)
{
	if (!local_bounds.valid) {
		return {};
	}
	const std::array<vec3, 8> local_corners = vertices_for_bounds(local_bounds.min, local_bounds.max);
	std::array<vec3, 8> world_corners{};
	for (std::size_t index = 0; index < local_corners.size(); ++index) {
		world_corners[index] = vec3(transform * vec4(local_corners[index], 1.0f));
	}
	return bounds_for_vertices(world_corners);
}

struct TriangleMeshOverlapAccumulator {
	vec3 accumulated_normal{0.0f};
	vec3 accumulated_contact{0.0f};
	float maximum_penetration = 0.0f;
	int contact_count = 0;
	vec3 preferred_normal{0.0f};
	vec3 preferred_contact{0.0f};
	float preferred_penetration = 0.0f;
	float preferred_score = 0.0f;
	bool has_preferred_contact = false;
	vec3 preferred_direction_normalized{0.0f};
};

TriangleMeshOverlapAccumulator create_triangle_mesh_overlap_accumulator(const vec3 *preferred_direction)
{
	TriangleMeshOverlapAccumulator accumulator;
	if (preferred_direction != nullptr && glm::length(*preferred_direction) > kMinDelta) {
		accumulator.preferred_direction_normalized = glm::normalize(*preferred_direction);
	}
	return accumulator;
}

void accumulate_triangle_pair_overlap(const CollisionTriangle &triangle_a,
                                      const CollisionTriangle &triangle_b,
                                      TriangleMeshOverlapAccumulator *accumulator)
{
	if (accumulator == nullptr ||
	    !triangle_a.valid ||
	    !triangle_b.valid ||
	    !compute_aabb_overlap(triangle_a.bounds, triangle_b.bounds, nullptr, nullptr, nullptr)) {
		return;
	}

	vec3 pair_normal(0.0f);
	vec3 pair_contact(0.0f);
	float pair_penetration = 0.0f;
	if (!compute_triangle_triangle_overlap(triangle_a,
	                                      triangle_b,
	                                      &pair_normal,
	                                      &pair_penetration,
	                                      &pair_contact)) {
		return;
	}

	accumulator->accumulated_normal += pair_normal * pair_penetration;
	accumulator->accumulated_contact += pair_contact;
	accumulator->maximum_penetration = std::max(accumulator->maximum_penetration, pair_penetration);
	++accumulator->contact_count;

	if (glm::length(accumulator->preferred_direction_normalized) > kMinDelta) {
		const float closing_score =
			-glm::dot(pair_normal, accumulator->preferred_direction_normalized) *
			std::max(pair_penetration, 0.001f);
		if (closing_score > 0.0f &&
		    (!accumulator->has_preferred_contact || closing_score > accumulator->preferred_score)) {
			accumulator->has_preferred_contact = true;
			accumulator->preferred_score = closing_score;
			accumulator->preferred_normal = pair_normal;
			accumulator->preferred_contact = pair_contact;
			accumulator->preferred_penetration = pair_penetration;
		}
	}
}

bool finalize_triangle_mesh_overlap(const CollisionGeometry &a,
                                    const CollisionGeometry &b,
                                    const TriangleMeshOverlapAccumulator &accumulator,
                                    vec3 *normal,
                                    float *penetration,
                                    vec3 *contact_point)
{
	if (accumulator.contact_count == 0) {
		return false;
	}

	if (accumulator.has_preferred_contact) {
		if (normal != nullptr) {
			*normal = accumulator.preferred_normal;
		}
		if (penetration != nullptr) {
			*penetration = accumulator.preferred_penetration;
		}
		if (contact_point != nullptr) {
			*contact_point = accumulator.preferred_contact;
		}
		return true;
	}

	vec3 resolved_normal = accumulator.accumulated_normal;
	if (glm::length(resolved_normal) <= kMinDelta) {
		resolved_normal = b.center_of_mass - a.center_of_mass;
		if (glm::length(resolved_normal) <= kMinDelta) {
			resolved_normal = b.bounds.center - a.bounds.center;
		}
		if (glm::length(resolved_normal) <= kMinDelta) {
			resolved_normal = vec3(0.0f, 1.0f, 0.0f);
		}
	}
	resolved_normal = glm::normalize(resolved_normal);

	if (normal != nullptr) {
		*normal = resolved_normal;
	}
	if (penetration != nullptr) {
		*penetration = accumulator.maximum_penetration;
	}
	if (contact_point != nullptr) {
		*contact_point = accumulator.accumulated_contact /
		                 static_cast<float>(accumulator.contact_count);
	}
	return true;
}

bool compute_triangle_mesh_overlap_with_bvh(const CollisionGeometry &mesh_geometry,
                                            const CollisionGeometry &other,
                                            vec3 *normal,
                                            float *penetration,
                                            vec3 *contact_point,
                                            const vec3 *preferred_direction)
{
	if (mesh_geometry.source_mesh == nullptr) {
		return false;
	}
	const std::vector<Mesh::CollisionBvhNode> &nodes =
		mesh_geometry.source_mesh->getCollisionBvhNodes();
	const std::vector<int> &triangle_indices =
		mesh_geometry.source_mesh->getCollisionBvhTriangleIndices();
	if (nodes.empty() || triangle_indices.empty()) {
		return false;
	}

	TriangleMeshOverlapAccumulator accumulator =
		create_triangle_mesh_overlap_accumulator(preferred_direction);
	std::stack<int> node_stack;
	node_stack.push(0);
	while (!node_stack.empty()) {
		const int node_index = node_stack.top();
		node_stack.pop();
		if (node_index < 0 || static_cast<std::size_t>(node_index) >= nodes.size()) {
			continue;
		}
		const Mesh::CollisionBvhNode &node = nodes[static_cast<std::size_t>(node_index)];
		const PrimitiveBounds node_bounds =
			transform_mesh_bounds(node.bounds, mesh_geometry.source_transform);
		if (!compute_aabb_overlap(node_bounds, other.bounds, nullptr, nullptr, nullptr)) {
			continue;
		}

		if (!node.isLeaf()) {
			if (node.left >= 0) {
				node_stack.push(node.left);
			}
			if (node.right >= 0) {
				node_stack.push(node.right);
			}
			continue;
		}

		for (int offset = 0; offset < node.count; ++offset) {
			const int triangle_index =
				triangle_indices[static_cast<std::size_t>(node.start + offset)];
			if (triangle_index < 0 ||
			    static_cast<std::size_t>(triangle_index) >= mesh_geometry.triangles.size()) {
				continue;
			}
			const CollisionTriangle &triangle_a =
				mesh_geometry.triangles[static_cast<std::size_t>(triangle_index)];
			if (!triangle_a.valid ||
			    !compute_aabb_overlap(triangle_a.bounds, other.bounds, nullptr, nullptr, nullptr)) {
				continue;
			}
			for (const CollisionTriangle &triangle_b : other.triangles) {
				accumulate_triangle_pair_overlap(triangle_a, triangle_b, &accumulator);
			}
		}
	}

	return finalize_triangle_mesh_overlap(mesh_geometry,
	                                      other,
	                                      accumulator,
	                                      normal,
	                                      penetration,
	                                      contact_point);
}

bool compute_triangle_triangle_overlap(const CollisionTriangle &a,
                                       const CollisionTriangle &b,
                                       vec3 *normal,
                                       float *penetration,
                                       vec3 *contact_point)
{
	if (!a.valid || !b.valid ||
	    !compute_aabb_overlap(a.bounds, b.bounds, nullptr, nullptr, nullptr)) {
		return false;
	}

	const std::array<vec3, 3> edges_a = {
		a.vertices[1] - a.vertices[0],
		a.vertices[2] - a.vertices[1],
		a.vertices[0] - a.vertices[2]};
	const std::array<vec3, 3> edges_b = {
		b.vertices[1] - b.vertices[0],
		b.vertices[2] - b.vertices[1],
		b.vertices[0] - b.vertices[2]};

	std::vector<vec3> axes;
	axes.reserve(17);
	append_unique_axis(&axes, a.normal);
	append_unique_axis(&axes, b.normal);
	for (const vec3 &edge : edges_a) {
		append_unique_axis(&axes, glm::cross(a.normal, edge));
	}
	for (const vec3 &edge : edges_b) {
		append_unique_axis(&axes, glm::cross(b.normal, edge));
	}
	for (const vec3 &edge_a : edges_a) {
		for (const vec3 &edge_b : edges_b) {
			append_unique_axis(&axes, glm::cross(edge_a, edge_b));
		}
	}
	if (axes.empty()) {
		return false;
	}

	const vec3 center_delta = b.centroid - a.centroid;
	float minimum_overlap = std::numeric_limits<float>::max();
	vec3 minimum_axis(0.0f);

	for (const vec3 &axis : axes) {
		float min_a = 0.0f;
		float max_a = 0.0f;
		float min_b = 0.0f;
		float max_b = 0.0f;
		project_vertices_onto_axis(a.vertices, axis, &min_a, &max_a);
		project_vertices_onto_axis(b.vertices, axis, &min_b, &max_b);
		const float overlap = std::min(max_a, max_b) - std::max(min_a, min_b);
		if (overlap <= 0.0f) {
			return false;
		}

		vec3 oriented_axis = axis;
		if (glm::dot(center_delta, oriented_axis) < 0.0f) {
			oriented_axis = -oriented_axis;
		}
		if (overlap < minimum_overlap) {
			minimum_overlap = overlap;
			minimum_axis = oriented_axis;
		}
	}

	if (normal != nullptr) {
		*normal = minimum_axis;
	}
	if (penetration != nullptr) {
		*penetration = minimum_overlap;
	}
	if (contact_point != nullptr) {
		const vec3 point_a = support_feature_center(a.vertices, minimum_axis);
		const vec3 point_b = support_feature_center(b.vertices, -minimum_axis);
		*contact_point = (point_a + point_b) * 0.5f;
	}
	return true;
}

bool compute_triangle_mesh_overlap(const CollisionGeometry &a,
                                   const CollisionGeometry &b,
                                   vec3 *normal,
                                   float *penetration,
                                   vec3 *contact_point,
                                   const vec3 *preferred_direction = nullptr)
{
	if (!compute_aabb_overlap(a.bounds, b.bounds, nullptr, nullptr, nullptr)) {
		return false;
	}

	if (a.source_mesh != nullptr &&
	    (!b.source_mesh || a.triangles.size() >= b.triangles.size()) &&
	    compute_triangle_mesh_overlap_with_bvh(a, b, normal, penetration, contact_point, preferred_direction)) {
		return true;
	}

	if (b.source_mesh != nullptr &&
	    compute_triangle_mesh_overlap_with_bvh(b, a, normal, penetration, contact_point, preferred_direction)) {
		if (normal != nullptr) {
			*normal = -*normal;
		}
		return true;
	}

	TriangleMeshOverlapAccumulator accumulator =
		create_triangle_mesh_overlap_accumulator(preferred_direction);
	for (const CollisionTriangle &triangle_a : a.triangles) {
		if (!triangle_a.valid ||
		    !compute_aabb_overlap(triangle_a.bounds, b.bounds, nullptr, nullptr, nullptr)) {
			continue;
		}
		for (const CollisionTriangle &triangle_b : b.triangles) {
			accumulate_triangle_pair_overlap(triangle_a, triangle_b, &accumulator);
		}
	}
	return finalize_triangle_mesh_overlap(a, b, accumulator, normal, penetration, contact_point);
}

bool compute_convex_hull_overlap(const CollisionGeometry &a,
	                             const CollisionGeometry &b,
	                             vec3 *normal,
	                             float *penetration,
	                             vec3 *contact_point)
{
	if (!compute_aabb_overlap(a.bounds, b.bounds, nullptr, nullptr, nullptr) ||
	    a.convex_vertices.empty() || b.convex_vertices.empty()) {
		return false;
	}

	const ConvexCollisionShape first_shape(a.convex_vertices, a.center_of_mass);
	const ConvexCollisionShape second_shape(b.convex_vertices, b.center_of_mass);
	const CollisionContact contact =
		ConvexCollisionDetector().detect(first_shape, second_shape);
	if (!contact.intersects()) {
		return false;
	}
	if (normal != nullptr) {
		*normal = contact.normal();
	}
	if (penetration != nullptr) {
		*penetration = contact.penetration();
	}
	if (contact_point != nullptr) {
		*contact_point = contact.point();
	}
	return true;
}

bool compute_polyhedron_overlap(const CollisionGeometry &a,
                                const CollisionGeometry &b,
                                vec3 *normal,
                                float *penetration,
                                vec3 *contact_point)
{
	if (!compute_aabb_overlap(a.bounds, b.bounds, nullptr, nullptr, nullptr)) {
		return false;
	}

	if (!a.valid || !b.valid) {
		return compute_aabb_overlap(a.bounds, b.bounds, normal, penetration, contact_point);
	}
	if (a.obb.valid && b.obb.valid) {
		return compute_obb_overlap(a, b, normal, penetration, contact_point);
	}

	std::vector<vec3> axes;
	axes.reserve(a.face_axes.size() + b.face_axes.size() +
	             a.edge_directions.size() * b.edge_directions.size());

	for (const vec3 &axis : a.face_axes) {
		append_unique_axis(&axes, axis);
	}
	for (const vec3 &axis : b.face_axes) {
		append_unique_axis(&axes, axis);
	}
	for (const vec3 &edge_a : a.edge_directions) {
		for (const vec3 &edge_b : b.edge_directions) {
			append_unique_axis(&axes, glm::cross(edge_a, edge_b));
		}
	}

	if (axes.empty()) {
		return compute_aabb_overlap(a.bounds, b.bounds, normal, penetration, contact_point);
	}

	const vec3 center_delta = b.centroid - a.centroid;
	float minimum_overlap = std::numeric_limits<float>::max();
	vec3 minimum_axis(0.0f);

	for (const vec3 &axis : axes) {
		float min_a = 0.0f;
		float max_a = 0.0f;
		float min_b = 0.0f;
		float max_b = 0.0f;
		project_vertices_onto_axis(a.vertices, axis, &min_a, &max_a);
		project_vertices_onto_axis(b.vertices, axis, &min_b, &max_b);
		const float overlap = std::min(max_a, max_b) - std::max(min_a, min_b);
		if (overlap <= 0.0f) {
			return false;
		}

		vec3 oriented_axis = axis;
		if (glm::dot(center_delta, oriented_axis) < 0.0f) {
			oriented_axis = -oriented_axis;
		}
		if (overlap < minimum_overlap) {
			minimum_overlap = overlap;
			minimum_axis = oriented_axis;
		}
	}

	if (normal != nullptr) {
		*normal = minimum_axis;
	}
	if (penetration != nullptr) {
		*penetration = minimum_overlap;
	}
	if (contact_point != nullptr) {
		const vec3 point_a = support_vertex(a.vertices, minimum_axis);
		const vec3 point_b = support_vertex(b.vertices, -minimum_axis);
		*contact_point = (point_a + point_b) * 0.5f;
	}
	return true;
}

vec3 inverse_inertia_for_box(const PrimitiveBounds &bounds, float mass, bool immovable)
{
	if (immovable || mass <= kMinimumMass) {
		return vec3(0.0f);
	}

	const vec3 full_extents = glm::max(bounds.half_extents * 2.0f, vec3(0.0001f));
	const float ix = (mass / 12.0f) * (full_extents.y * full_extents.y + full_extents.z * full_extents.z);
	const float iy = (mass / 12.0f) * (full_extents.x * full_extents.x + full_extents.z * full_extents.z);
	const float iz = (mass / 12.0f) * (full_extents.x * full_extents.x + full_extents.y * full_extents.y);

	return vec3(ix > kMinDelta ? 1.0f / ix : 0.0f,
	            iy > kMinDelta ? 1.0f / iy : 0.0f,
	            iz > kMinDelta ? 1.0f / iz : 0.0f);
}

vec3 gravity_acceleration_for_instance(const PrimitiveInstance &instance, const vec3 &gravity_acceleration)
{
	if (instance.immovable || instance.mass <= kMinimumMass) {
		return vec3(0.0f);
	}
	return gravity_acceleration;
}

bool is_outside_simulation_bounds(const PrimitiveBounds &bounds)
{
	if (!bounds.valid) {
		return false;
	}

	return bounds.max.x < -kSimulationBoundsExtent || bounds.min.x > kSimulationBoundsExtent ||
	       bounds.max.y < -kSimulationBoundsExtent || bounds.min.y > kSimulationBoundsExtent ||
	       bounds.max.z < -kSimulationBoundsExtent || bounds.min.z > kSimulationBoundsExtent;
}

bool is_outside_bounds(const PrimitiveBounds &bounds, const PrimitiveBounds &container_bounds)
{
	if (!bounds.valid || !container_bounds.valid) {
		return false;
	}

	return bounds.max.x < container_bounds.min.x || bounds.min.x > container_bounds.max.x ||
	       bounds.max.y < container_bounds.min.y || bounds.min.y > container_bounds.max.y ||
	       bounds.max.z < container_bounds.min.z || bounds.min.z > container_bounds.max.z;
}

void stop_simulation(PrimitiveInstance *instance)
{
	if (instance == nullptr || instance->immovable) {
		return;
	}

	instance->simulation_active = false;
	instance->sleeping = false;
	instance->sleep_timer = 0.0f;
	instance->velocity = vec3(0.0f);
	instance->rotational_velocity = vec3(0.0f);
}

void remove_from_scene(PrimitiveInstance *instance)
{
	if (instance == nullptr) {
		return;
	}

	instance->removed = true;
	instance->simulation_active = false;
	instance->sleeping = false;
	instance->sleep_timer = 0.0f;
	instance->velocity = vec3(0.0f);
	instance->rotational_velocity = vec3(0.0f);
}

bool stop_simulation_if_outside_bounds(PrimitiveInstance *instance, const CollisionGeometry &geometry)
{
	if (instance == nullptr || !is_dynamic_simulation_active(*instance)) {
		return false;
	}
	if (!is_outside_simulation_bounds(geometry.bounds)) {
		return false;
	}

	stop_simulation(instance);
	return true;
}

bool remove_dynamic_cube_if_outside_scene_bounds(PrimitiveInstance *instance,
                                                 const CollisionGeometry &geometry,
                                                 const PrimitiveBounds &scene_bounds)
{
	if (instance == nullptr || !is_dynamic_simulation_active(*instance)) {
		return false;
	}
	if (!is_cube_like(instance->type)) {
		return false;
	}
	if (!is_outside_bounds(geometry.bounds, scene_bounds)) {
		return false;
	}

	remove_from_scene(instance);
	return true;
}

void apply_rotational_impulse(PrimitiveInstance *instance,
                              const CollisionGeometry &geometry,
                              const vec3 &contact_point,
                              const vec3 &impulse)
{
	if (instance == nullptr || instance->immovable) {
		return;
	}

	const vec3 offset = contact_point - geometry.center_of_mass;
	const vec3 angular_impulse = glm::cross(offset, impulse);
	const vec3 inverse_inertia =
		inverse_inertia_for_box(geometry.bounds, instance->mass, instance->immovable);
	instance->rotational_velocity += glm::degrees(angular_impulse * inverse_inertia);
	clamp_rotational_velocity(instance);
}

bool resolve_collision(Context *context,
                       PrimitiveInstance *a,
                       PrimitiveInstance *b,
                       const CollisionGeometry &geometry_a,
                       const CollisionGeometry &geometry_b)
{
	if (a == nullptr || b == nullptr || !is_collision_candidate(*a) || !is_collision_candidate(*b)) {
		return false;
	}
	if (a->immovable && b->immovable) {
		return false;
	}
	if (!passes_distance_rejection_test(geometry_a, geometry_b)) {
		return false;
	}
	const PrimitiveBounds &bounds_a = geometry_a.bounds;
	const PrimitiveBounds &bounds_b = geometry_b.bounds;
	if (!compute_aabb_overlap(bounds_a, bounds_b, nullptr, nullptr, nullptr)) {
		return false;
	}
	const vec3 relative_velocity = b->velocity - a->velocity;
	vec3 normal(0.0f);
	vec3 contact_point(0.0f);
	float penetration = 0.0f;
	bool overlaps = false;
	const bool a_is_uniform_sphere = is_default_uniform_sphere(*a, geometry_a);
	const bool b_is_uniform_sphere = is_default_uniform_sphere(*b, geometry_b);
	if (a_is_uniform_sphere && b_is_uniform_sphere) {
		overlaps = compute_sphere_sphere_overlap(
			geometry_a, geometry_b, relative_velocity, &normal, &penetration, &contact_point);
	} else if (a_is_uniform_sphere && geometry_b.kind == CollisionGeometryKind::BoxHull) {
		overlaps = compute_sphere_box_overlap(
			geometry_a, geometry_b, true, &normal, &penetration, &contact_point);
	} else if (b_is_uniform_sphere && geometry_a.kind == CollisionGeometryKind::BoxHull) {
		overlaps = compute_sphere_box_overlap(
			geometry_b, geometry_a, false, &normal, &penetration, &contact_point);
	} else if (geometry_a.kind == CollisionGeometryKind::TriangleMesh ||
	    geometry_b.kind == CollisionGeometryKind::TriangleMesh) {
		overlaps = compute_triangle_mesh_overlap(geometry_a,
		                                       geometry_b,
		                                       &normal,
		                                       &penetration,
		                                       &contact_point,
		                                       &relative_velocity);
	} else if (geometry_a.kind == CollisionGeometryKind::ConvexHull ||
	           geometry_b.kind == CollisionGeometryKind::ConvexHull) {
		overlaps = compute_convex_hull_overlap(geometry_a,
		                                     geometry_b,
		                                     &normal,
		                                     &penetration,
		                                     &contact_point);
	} else {
		overlaps = compute_polyhedron_overlap(
			geometry_a, geometry_b, &normal, &penetration, &contact_point);
	}
	if (overlaps &&
	    is_default_uniform_cylinder(*a, geometry_a) &&
	    geometry_b.kind == CollisionGeometryKind::BoxHull) {
		stabilize_cylinder_box_cap_contact(
			geometry_a, geometry_b, true, &normal, &penetration, &contact_point);
	} else if (overlaps &&
	           is_default_uniform_cylinder(*b, geometry_b) &&
	           geometry_a.kind == CollisionGeometryKind::BoxHull) {
		stabilize_cylinder_box_cap_contact(
			geometry_b, geometry_a, false, &normal, &penetration, &contact_point);
	}
	if (!overlaps) {
		return false;
	}

	const float effective_penetration = penetration;
	if (effective_penetration <= 0.0f) {
		return false;
	}

	const float inverse_mass_a = a->immovable ? 0.0f : 1.0f / std::max(a->mass, kMinimumMass);
	const float inverse_mass_b = b->immovable ? 0.0f : 1.0f / std::max(b->mass, kMinimumMass);
	const float inverse_mass_sum = inverse_mass_a + inverse_mass_b;
	if (inverse_mass_sum <= 0.0f) {
		return true;
	}

	const float velocity_along_normal = glm::dot(relative_velocity, normal);
	const float closing_speed = std::max(0.0f, -velocity_along_normal);
	if (collision_contact_requires_wake(closing_speed, effective_penetration)) {
		if (a->sleeping) {
			wake_instance(a);
		}
		if (b->sleeping) {
			wake_instance(b);
		}
	}
	if (velocity_along_normal < 0.0f) {
		const float restitution = collision_restitution_for_closing_speed(closing_speed);
		const float impulse_magnitude =
			-(1.0f + restitution) * velocity_along_normal / inverse_mass_sum;
		const vec3 impulse = impulse_magnitude * normal;
		const bool resting_contact = restitution == 0.0f;
		if (context != nullptr && !resting_contact) {
			context->emitCollisionParticles(contact_point, normal, relative_velocity, impulse_magnitude);
		}
		if (!a->immovable) {
			a->velocity -= impulse * inverse_mass_a;
			if (!resting_contact) {
				apply_rotational_impulse(a, geometry_a, contact_point, -impulse);
			}
		}
		if (!b->immovable) {
			b->velocity += impulse * inverse_mass_b;
			if (!resting_contact) {
				apply_rotational_impulse(b, geometry_b, contact_point, impulse);
			}
		}
	}

	const float correction_magnitude =
		std::max(effective_penetration - kPenetrationSlop, 0.0f) * kPenetrationCorrection / inverse_mass_sum;
	const vec3 correction = correction_magnitude * normal;
	if (!a->immovable) {
		apply_world_translation(a, -correction * inverse_mass_a);
	}
	if (!b->immovable) {
		apply_world_translation(b, correction * inverse_mass_b);
	}
	return true;
}

void append_primitive_vertices(const PrimitiveTransformCache &cache,
	                           float texscale,
	                           const GLfloat *vertex_data,
	                           std::vector<GLfloat> *buffer)
{
	const std::size_t base_offset = buffer->size();
	buffer->resize(base_offset + (kPrimitiveVertexCount * kVertexStride));

	for (int vertex_index = 0; vertex_index < kPrimitiveVertexCount; ++vertex_index) {
		const int source_offset = vertex_index * kVertexStride;
		const std::size_t destination_offset =
			base_offset + static_cast<std::size_t>(source_offset);

		const glm::vec4 base_vertex(vertex_data[source_offset],
		                            vertex_data[source_offset + 1],
		                            vertex_data[source_offset + 2],
		                            1.0f);
		glm::mat3 local_normal_transform(1.0f);
		const glm::vec4 local_vertex = transform_local_vertex(cache, base_vertex, &local_normal_transform);
		const glm::mat4 *transform = nullptr;
		const glm::mat3 *normal_transform = nullptr;
		select_world_transform(cache, base_vertex, &transform, &normal_transform);
		const glm::vec4 vertex = (*transform) * local_vertex;
		(*buffer)[destination_offset] = vertex.x;
		(*buffer)[destination_offset + 1] = vertex.y;
		(*buffer)[destination_offset + 2] = vertex.z;

		const glm::vec3 local_normal = glm::normalize(
			local_normal_transform *
			glm::vec3(vertex_data[source_offset + 3],
			          vertex_data[source_offset + 4],
			          vertex_data[source_offset + 5]));
		const glm::vec3 normal = glm::normalize((*normal_transform) * local_normal);
		const vec2 uv =
			compute_face_aligned_uv(vec3(local_vertex), local_normal, *transform, texscale);
		(*buffer)[destination_offset + 3] = normal.x;
		(*buffer)[destination_offset + 4] = normal.y;
		(*buffer)[destination_offset + 5] = normal.z;
		(*buffer)[destination_offset + 6] = uv.x;
		(*buffer)[destination_offset + 7] = uv.y;
	}
}

void append_mesh_vertices(const PrimitiveInstance &instance,
                          const Mesh &mesh,
                          std::vector<GLfloat> *buffer)
{
	if (buffer == nullptr || mesh.faces.empty()) {
		return;
	}

	const glm::mat4 transform = instance.primary_transform;
	const glm::mat3 normal_transform = safe_normal_transform(transform);

	for (std::size_t face_index = 0; face_index < mesh.faces.size(); ++face_index) {
		const glm::ivec3 &face = mesh.faces[face_index];
		const glm::vec3 local_vertices[3] = {
			mesh.vertices[static_cast<std::size_t>(face.x)],
			mesh.vertices[static_cast<std::size_t>(face.y)],
			mesh.vertices[static_cast<std::size_t>(face.z)]};
		glm::vec3 local_normal(0.0f, 1.0f, 0.0f);
		if (face_index < mesh.face_normals.size() &&
		    glm::length(mesh.face_normals[face_index]) > kMinDelta) {
			local_normal = glm::normalize(mesh.face_normals[face_index]);
		} else {
			const glm::vec3 derived_normal =
				glm::cross(local_vertices[1] - local_vertices[0],
				           local_vertices[2] - local_vertices[0]);
			if (glm::length(derived_normal) > kMinDelta) {
				local_normal = glm::normalize(derived_normal);
			}
		}
		for (int vertex_index = 0; vertex_index < 3; ++vertex_index) {
			const std::size_t mesh_vertex_index = static_cast<std::size_t>(face[vertex_index]);
			const glm::vec3 &local_vertex = local_vertices[vertex_index];
			glm::vec3 local_vertex_normal = local_normal;
			if (mesh.normals.size() == mesh.vertices.size() &&
			    glm::length(mesh.normals[mesh_vertex_index]) > kMinDelta) {
				local_vertex_normal = glm::normalize(mesh.normals[mesh_vertex_index]);
			}
			const glm::vec3 transformed_normal = normal_transform * local_vertex_normal;
			const glm::vec3 world_normal =
				glm::length(transformed_normal) > kMinDelta
					? glm::normalize(transformed_normal)
					: glm::vec3(0.0f, 1.0f, 0.0f);
			const glm::vec4 world_vertex = transform * glm::vec4(local_vertex, 1.0f);
			vec2 uv;
			if (mesh.texcoords.size() == mesh.vertices.size()) {
				const glm::vec3 &mesh_texcoord = mesh.texcoords[mesh_vertex_index];
				uv = vec2(mesh_texcoord.x, mesh_texcoord.y) *
				     effective_texture_tile_scale(instance.texscale);
			} else {
				uv = compute_face_aligned_uv(
					local_vertex,
					local_vertex_normal,
					transform,
					instance.texscale);
			}
			buffer->push_back(world_vertex.x);
			buffer->push_back(world_vertex.y);
			buffer->push_back(world_vertex.z);
			buffer->push_back(world_normal.x);
			buffer->push_back(world_normal.y);
			buffer->push_back(world_normal.z);
			buffer->push_back(uv.x);
			buffer->push_back(uv.y);
		}
	}
}

std::size_t vertex_count_for_instance(const PrimitiveInstance &instance)
{
	if (instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh()) {
		return instance.resolved_geometry->triangleVertexCount();
	}
	if (instance.resolved_geometry && instance.resolved_geometry->usesCubeVertexTemplate()) {
		return kPrimitiveVertexCount;
	}
	return 0u;
}

void append_instance_vertices(const PrimitiveInstance &instance,
                              const GLfloat *vertex_data,
                              std::vector<GLfloat> *buffer)
{
	if (instance.resolved_geometry && instance.resolved_geometry->hasTriangleMesh()) {
		append_mesh_vertices(instance, instance.resolved_geometry->triangleMesh(), buffer);
		return;
	}
	if (instance.resolved_geometry && instance.resolved_geometry->usesCubeVertexTemplate()) {
		append_primitive_vertices(build_transform_cache(instance), instance.texscale, vertex_data, buffer);
	}
}

Primitive *primitive_for_type(Context *context, const std::string &type)
{
	if (context == nullptr) {
		return nullptr;
	}
	if (type == "Cube") {
		return context->Cube;
	}
	if (type == "CubeX") {
		return context->CubeX;
	}
	if (type == "CubeY") {
		return context->CubeY;
	}
	if (type == "CubeZ") {
		return context->CubeZ;
	}
	if (type == "Cylinder") {
		return context->Cylinder;
	}
	if (type == "Sphere") {
		return context->Sphere;
	}
	if (type == "AxialProfile") {
		return context->AxialProfile;
	}
	return nullptr;
}

float cube_like_instance_volume(const PrimitiveInstance &instance)
{
	static const std::array<vec3, 8> kBaseCubeCorners = {
		vec3(0.5f, 0.0f, 0.5f),
		vec3(0.5f, 1.0f, 0.5f),
		vec3(-0.5f, 0.0f, 0.5f),
		vec3(-0.5f, 1.0f, 0.5f),
		vec3(0.5f, 0.0f, -0.5f),
		vec3(0.5f, 1.0f, -0.5f),
		vec3(-0.5f, 0.0f, -0.5f),
		vec3(-0.5f, 1.0f, -0.5f)};
	static const std::array<glm::ivec3, 12> kCubeTriangles = {
		glm::ivec3(0, 3, 2), glm::ivec3(0, 1, 3),
		glm::ivec3(5, 4, 7), glm::ivec3(4, 6, 7),
		glm::ivec3(4, 0, 6), glm::ivec3(0, 2, 6),
		glm::ivec3(5, 3, 1), glm::ivec3(5, 7, 3),
		glm::ivec3(4, 1, 0), glm::ivec3(5, 1, 4),
		glm::ivec3(2, 3, 6), glm::ivec3(6, 3, 7)};

	const PrimitiveTransformCache cache = build_transform_cache(instance, false);
	std::array<glm::dvec3, 8> transformed_vertices{};
	for (std::size_t index = 0; index < kBaseCubeCorners.size(); ++index) {
		transformed_vertices[index] = glm::dvec3(transform_world_vertex(cache, kBaseCubeCorners[index]));
	}

	double signed_volume = 0.0;
	for (const glm::ivec3 &triangle : kCubeTriangles) {
		const glm::dvec3 &a = transformed_vertices[static_cast<std::size_t>(triangle.x)];
		const glm::dvec3 &b = transformed_vertices[static_cast<std::size_t>(triangle.y)];
		const glm::dvec3 &c = transformed_vertices[static_cast<std::size_t>(triangle.z)];
		signed_volume += glm::dot(a, glm::cross(b, c)) / 6.0;
	}

	return static_cast<float>(std::fabs(signed_volume));
}

bool resolve_instance_mass(const PrimitiveInstance &instance,
                           float pending_mass,
                           float pending_density,
                           bool has_pending_density,
                           float *resolved_mass,
                           std::string *diagnostic)
{
	if(resolved_mass == nullptr)return false;
	*resolved_mass = std::max(pending_mass, kMinimumMass);
	if (!has_pending_density) {
		return true;
	}

	float volume = 0.0f;
	if (is_cube_like(instance.type)) {
		volume = cube_like_instance_volume(instance);
	}
	else if (instance.resolved_geometry &&
	         instance.resolved_geometry->volumeEvidence().hasVolume()) {
		const float local_volume =
			instance.resolved_geometry->volumeEvidence().localVolume();
		const float transform_volume_scale =
			std::fabs(glm::determinant(glm::mat3(instance.primary_transform)));
		volume = local_volume * transform_volume_scale;
	}
	else {
		if(diagnostic != nullptr){
			*diagnostic = "Density-derived mass is undefined for an open or non-watertight primitive.";
		}
		return false;
	}
	if (volume <= kMinDelta) {
		if(diagnostic != nullptr){
			*diagnostic = "Density-derived mass requires a finite nonzero transformed volume.";
		}
		return false;
	}

	*resolved_mass = std::max(pending_density * volume, kMinimumMass);
	return true;
}

} // namespace

void SceneObjectState::restoreInitialState()
{
	position = initial_position;
	velocity = initial_velocity;
	rotational_velocity = initial_rotational_velocity;
	mass = initial_mass;
	immovable = initial_immovable;
	simulation_active = initial_simulation_active;
	removed = initial_removed;
	sleep_timer = 0.0f;
	sleeping = false;
}

void SceneRenderableState::restoreInitialState()
{
	SceneObjectState::restoreInitialState();
}

void ScenePrimitiveInstance::restoreInitialState()
{
	SceneRenderableState::restoreInitialState();
	primary_transform = initial_primary_transform;
	secondary_transform = initial_secondary_transform;
	dual_scales = initial_dual_scales;
	dual_translations = initial_dual_translations;
	rotation_degrees = initial_rotation_degrees;
}

PrimitiveDefinition::PrimitiveDefinition(std::string type, GLuint texId, bool x, bool y, bool z)
{
	this->texId = texId;
	this->type = std::move(type);
	this->x = x;
	this->y = y;
	this->z = z;
}

void PrimitiveDefinition::draw(Scope *scope, int material_index, float alpha, float scaletex)
{
	if (scope == nullptr) {
		return;
	}

	if (type == "Cube" || type == "CubeX" || type == "CubeY" || type == "CubeZ") {
		const float x = std::fabs(scope->size.x);
		const float y = std::fabs(scope->size.y);
		const float z = std::fabs(scope->size.z);
		const float X = scope->position.x;
		const float Y = scope->position.y;
		const float Z = scope->position.z;
		glm::vec3 local_offset(0.0f, -0.5f, 0.0f);
		if (type == "CubeX") {
			local_offset = glm::vec3(0.5f, -0.5f, 0.0f);
		} else if (type == "CubeY") {
			local_offset = glm::vec3(0.0f, 0.0f, 0.0f);
		} else if (type == "CubeZ") {
			local_offset = glm::vec3(0.0f, -0.5f, 0.5f);
		}
		draw_box(glm::vec3(x, y, z), glm::vec3(X, Y, Z), local_offset, material_index, scaletex, alpha);
	}
}

void SceneGenerationContext::addPrimitive(std::string type,
                           Scope *scope,
	                           const std::string &material_name,
	                           float alpha,
	                           float texscale,
	                           bool immovable,
	                           int source_start_line,
	                           int source_start_column,
	                           int source_end_line,
	                           int source_end_column,
	                           std::shared_ptr<const ShapeSpecification> shape_specification)
{
	if (primitive_instances.size() >= kMaxGeneratedPrimitiveCount) {
		errorout("Grammar execution error: primitive budget of " +
		         std::to_string(kMaxGeneratedPrimitiveCount) +
		         " was exceeded.");
		clearPendingPrimitiveState();
		return;
	}

	Primitive *primitive = primitive_for_type(this, type);
	if (primitive == nullptr && shape_specification) {
		primitive = ProceduralShape;
	}
	if (scope == nullptr) {
		clearPendingPrimitiveState();
		return;
	}

	std::string geometry_diagnostic;
	const std::shared_ptr<const ResolvedPrimitiveGeometry> resolved_geometry =
		primitive_geometry_resolver_->resolvePrimitive(
			type, shape_specification, &geometry_diagnostic);
	if (!resolved_geometry || primitive == nullptr) {
		if(!geometry_diagnostic.empty()){
			errorout("Grammar execution error: " + geometry_diagnostic);
		}
		clearPendingPrimitiveState();
		return;
	}

	PrimitiveInstance instance;
	instance.primitive = primitive;
	instance.type = std::move(type);
	instance.resolved_geometry = resolved_geometry;
	instance.shape_specification = std::move(shape_specification);
	instance.primary_transform = scope->getTransform();
	instance.secondary_transform = scope->getTransform2();
	instance.initial_primary_transform = instance.primary_transform;
	instance.initial_secondary_transform = instance.secondary_transform;
	instance.position = glm::vec3(instance.primary_transform[3]);
	instance.initial_position = instance.position;
	instance.size = transform_scale_magnitudes(instance.primary_transform);
	instance.size2 = transform_scale_magnitudes(instance.secondary_transform);
	instance.dual_scales = scope->getDualScales();
	instance.dual_translations = scope->getDualTranslations();
	instance.initial_dual_scales = instance.dual_scales;
	instance.initial_dual_translations = instance.dual_translations;
	instance.rotation_degrees = transform_euler_degrees(instance.primary_transform);
	instance.initial_rotation_degrees = instance.rotation_degrees;
	instance.velocity = immovable ? vec3(0.0f) : pending_velocity_;
	instance.initial_velocity = instance.velocity;
	instance.rotational_velocity = immovable ? vec3(0.0f) : pending_rotational_velocity_;
	instance.initial_rotational_velocity = instance.rotational_velocity;
	float resolved_mass = kDefaultMass;
	std::string mass_diagnostic;
	if(!resolve_instance_mass(instance,
	                          pending_mass_,
	                          pending_density_,
	                          has_pending_density_,
	                          &resolved_mass,
	                          &mass_diagnostic)){
		errorout("Grammar execution error: " + mass_diagnostic);
		clearPendingPrimitiveState();
		return;
	}
	instance.mass = immovable ? kDefaultMass : resolved_mass;
	instance.initial_mass = instance.mass;
	instance.source_start_line = source_start_line;
	instance.source_start_column = source_start_column;
	instance.source_end_line = source_end_line;
	instance.source_end_column = source_end_column;
	instance.material_index = static_cast<int>(resolveMaterialSlot(material_name));
	instance.material_name = material_names_[static_cast<std::size_t>(instance.material_index)];
	instance.alpha = alpha;
	instance.texscale = texscale;
	instance.sleep_timer = 0.0f;
	instance.immovable = immovable;
	instance.initial_immovable = immovable;
	instance.simulation_active = !immovable;
	instance.initial_simulation_active = instance.simulation_active;
	instance.removed = false;
	instance.initial_removed = false;
	instance.sleeping = false;
	primitive_instances.push_back(instance);

	PrimitiveInstance &stored_instance = primitive_instances.back();
	if (hasActiveSpatialObject()) {
		const SpatialSceneConstructionState::ActiveObjectScope &object_scope =
			spatial_scene_construction_state_->active_object_scopes.back();
		const float determinant =
			glm::determinant(glm::mat3(object_scope.authored_world_transform));
		if (std::fabs(determinant) <= 1.0e-8f) {
			spatial_scene_construction_state_->blocking_diagnostic =
				"Spatial object '" + object_scope.object_id.value() +
				"' has a non-invertible authored transform.";
		}
		else {
			std::string binding_diagnostic;
			const glm::mat4 primitive_local_to_object =
				glm::inverse(object_scope.authored_world_transform) *
				stored_instance.primary_transform;
			if (!spatial_scene_construction_state_->construction_context.bindPrimitive(
					primitive_instances.size() - 1,
					primitive_local_to_object,
					{},
					&binding_diagnostic)) {
				spatial_scene_construction_state_->blocking_diagnostic = binding_diagnostic;
			}
		}
	}
	const PrimitiveBounds instance_bounds = collision_geometry_for_instance(stored_instance).bounds;
	stored_instance.previous_primary_transform = stored_instance.primary_transform;
	stored_instance.previous_secondary_transform = stored_instance.secondary_transform;
	stored_instance.previous_bounds_center = instance_bounds.valid ? instance_bounds.center : stored_instance.position;
	stored_instance.has_previous_render_state = uses_dynamic_cube_preview_path(stored_instance);
	if (instance_bounds.valid) {
		if (!has_scene_bounds_) {
			scene_bounds_min_ = instance_bounds.min;
			scene_bounds_max_ = instance_bounds.max;
			has_scene_bounds_ = true;
		} else {
			scene_bounds_min_ = glm::min(scene_bounds_min_, instance_bounds.min);
			scene_bounds_max_ = glm::max(scene_bounds_max_, instance_bounds.max);
		}
	}
	clearPendingPrimitiveState();
}

bool SceneGenerationContext::beginSpatialObject(
	SpatialObjectIdentity identity,
	const std::string &container_object_id,
	CollisionParticipationPolicy collision_policy,
	std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr || current_scope == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "Spatial object construction requires an active scene scope.";
		}
		return false;
	}
	if (!spatial_scene_construction_state_->blocking_diagnostic.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = spatial_scene_construction_state_->blocking_diagnostic;
		}
		return false;
	}

	const glm::mat4 authored_world_transform = current_scope->getTransform();
	SpatialObjectId parent_object_id;
	if (!container_object_id.empty()) {
		parent_object_id = SpatialObjectId(container_object_id);
	}
	else if (!spatial_scene_construction_state_->active_object_scopes.empty()) {
		parent_object_id =
			spatial_scene_construction_state_->active_object_scopes.back().object_id;
	}
	if (!container_object_id.empty() &&
	    !spatial_scene_construction_state_->active_object_scopes.empty() &&
	    spatial_scene_construction_state_->active_object_scopes.back().object_id !=
		parent_object_id) {
		if (diagnostic != nullptr) {
			*diagnostic = "Spatial object container '" + container_object_id +
				"' does not match the active parent object scope '" +
				spatial_scene_construction_state_->active_object_scopes.back().object_id.value() +
				"'.";
		}
		return false;
	}

	glm::mat4 authored_local_transform = authored_world_transform;
	SpatialFrameReference parent_frame = SpatialFrameReference::root();
	if (!parent_object_id.empty()) {
		parent_frame = SpatialFrameReference::object(parent_object_id);
		const auto parent_transform =
			spatial_scene_construction_state_->declared_world_transforms.find(
				parent_object_id.value());
		if (parent_transform !=
		    spatial_scene_construction_state_->declared_world_transforms.end()) {
			const float parent_determinant =
				glm::determinant(glm::mat3(parent_transform->second));
			if (std::fabs(parent_determinant) <= 1.0e-8f) {
				if (diagnostic != nullptr) {
					*diagnostic = "Spatial container '" + parent_object_id.value() +
						"' has a non-invertible authored transform.";
				}
				return false;
			}
			authored_local_transform =
				glm::inverse(parent_transform->second) * authored_world_transform;
		}
	}

	const SpatialObjectId object_id = identity.objectId();
	const bool nested_with_declared_parent =
		!spatial_scene_construction_state_->active_object_scopes.empty() &&
		spatial_scene_construction_state_->active_object_scopes.back().object_id ==
			parent_object_id;
	SpatialFrameState frame_state(
		authored_local_transform,
		glm::mat4(1.0f),
		authored_world_transform,
		std::move(parent_frame),
		SpatialPoseUncertainty());
	if (!spatial_scene_construction_state_->construction_context.beginObject(
			std::move(identity),
			std::move(frame_state),
			SpatialBoundaryModel(),
			collision_policy,
			SpatialObjectState::Approximate,
			diagnostic)) {
		return false;
	}
	if (!parent_object_id.empty() && !nested_with_declared_parent) {
		spatial_scene_construction_state_->construction_context.addContainmentRelationship(
			SpatialContainmentRelationship(parent_object_id, object_id));
	}

	spatial_scene_construction_state_->declarations_started = true;
	spatial_scene_construction_state_->declared_world_transforms[object_id.value()] =
		authored_world_transform;
	spatial_scene_construction_state_->active_object_scopes.push_back(
		SpatialSceneConstructionState::ActiveObjectScope{
			object_id, authored_world_transform});
	return true;
}

bool SceneGenerationContext::addSpatialInterface(
	SpatialInterface interface,
	std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr) {
		if (diagnostic != nullptr) *diagnostic = "Spatial construction state is unavailable.";
		return false;
	}
	return spatial_scene_construction_state_->construction_context.addInterface(
		std::move(interface), diagnostic);
}

bool SceneGenerationContext::addSpatialConnection(
	SpatialConnection connection,
	std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr) {
		if (diagnostic != nullptr) *diagnostic = "Spatial construction state is unavailable.";
		return false;
	}
	spatial_scene_construction_state_->construction_context.addConnection(
		std::move(connection));
	return true;
}

bool SceneGenerationContext::addSpatialConstraint(
	std::shared_ptr<const SpatialConstraint> constraint,
	std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr || !constraint) {
		if (diagnostic != nullptr) *diagnostic = "Spatial constraint is unavailable.";
		return false;
	}
	spatial_scene_construction_state_->construction_context.addConstraint(
		std::move(constraint));
	return true;
}

bool SceneGenerationContext::endSpatialObject(std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr ||
	    spatial_scene_construction_state_->active_object_scopes.empty()) {
		if (diagnostic != nullptr) *diagnostic = "No spatial object scope is available to close.";
		return false;
	}
	if (!spatial_scene_construction_state_->construction_context.endObject(diagnostic)) {
		return false;
	}
	spatial_scene_construction_state_->active_object_scopes.pop_back();
	return true;
}

bool SceneGenerationContext::finalizeSpatialBuildingModel(std::string *diagnostic)
{
	if (spatial_scene_construction_state_ == nullptr ||
	    !spatial_scene_construction_state_->declarations_started) {
		return true;
	}
	if (!spatial_scene_construction_state_->blocking_diagnostic.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = spatial_scene_construction_state_->blocking_diagnostic;
		}
		return false;
	}
	if (!spatial_scene_construction_state_->active_object_scopes.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Spatial object construction ended with unclosed object scopes.";
		}
		return false;
	}

	const SpatialBuildingModelConstructionResult construction_result =
		spatial_scene_construction_state_->construction_context.finalize(
			primitive_instances.size());
	if (!construction_result.succeeded()) {
		if (diagnostic != nullptr) {
			*diagnostic = construction_result.validationReport().firstDiagnostic();
		}
		return false;
	}
	const SpatialAssemblyResolutionResult resolution_result =
		SpatialAssemblyResolutionService().resolve(
			*construction_result.model(), this);
	if (!resolution_result.succeeded()) {
		if (diagnostic != nullptr) {
			*diagnostic = resolution_result.validationReport().firstDiagnostic();
		}
		return false;
	}
	const std::shared_ptr<const SpatialBuildingModel> resolved_model =
		resolution_result.model();
	std::shared_ptr<const SmallModernBuildingModel> building_model;
	const SmallModernBuildingKnowledgeCatalog knowledge_catalog;
	if (knowledge_catalog.matchesSpatialModel(*resolved_model)) {
		const SmallModernBuildingModelConstructionResult building_result =
			SmallModernBuildingModelConstructionService().construct(
				resolved_model, knowledge_catalog.createConstructionRequest());
		if (!building_result.succeeded()) {
			if (diagnostic != nullptr) {
				*diagnostic = building_result.validationReport().firstDiagnostic();
			}
			return false;
		}
		building_model = building_result.model();
	}
	spatial_scene_construction_state_->resolved_model = resolved_model;
	spatial_scene_construction_state_->building_model = std::move(building_model);
	return true;
}

bool SceneGenerationContext::hasActiveSpatialObject() const
{
	return spatial_scene_construction_state_ != nullptr &&
	       !spatial_scene_construction_state_->active_object_scopes.empty();
}

std::string SceneGenerationContext::currentSpatialObjectId() const
{
	if (!hasActiveSpatialObject()) return {};
	return spatial_scene_construction_state_->active_object_scopes.back().object_id.value();
}

const std::shared_ptr<const SpatialBuildingModel> &
SceneGenerationContext::spatialBuildingModel() const
{
	static const std::shared_ptr<const SpatialBuildingModel> empty_model;
	return spatial_scene_construction_state_ != nullptr
		? spatial_scene_construction_state_->resolved_model
		: empty_model;
}

const std::shared_ptr<const SmallModernBuildingModel> &
SceneGenerationContext::smallModernBuildingModel() const
{
	static const std::shared_ptr<const SmallModernBuildingModel> empty_model;
	return spatial_scene_construction_state_ != nullptr
		? spatial_scene_construction_state_->building_model
		: empty_model;
}

GLfloat *SceneGenerationContext::calc(const GLfloat *vertex_data, int material_index, int *out_count)
{
	if (out_count != nullptr) {
		*out_count = 0;
	}
	if (material_index < 0) {
		return nullptr;
	}

	std::vector<std::vector<GLfloat>> buffers;
	std::vector<int> counts;
	buildMaterialBuffers(vertex_data,
	                    static_cast<std::size_t>(material_index + 1),
	                    &buffers,
	                    &counts);
	if (static_cast<std::size_t>(material_index) >= buffers.size() ||
	    static_cast<std::size_t>(material_index) >= counts.size()) {
		return nullptr;
	}
	if (out_count != nullptr) {
		*out_count = counts[material_index];
	}
	if (buffers[material_index].empty()) {
		return nullptr;
	}

	GLfloat *vertex_buffer = new GLfloat[buffers[material_index].size()];
	std::copy(buffers[material_index].begin(), buffers[material_index].end(), vertex_buffer);
	return vertex_buffer;
}

void SceneGenerationContext::buildMaterialBuffers(const GLfloat *vertex_data,
	                               std::size_t material_count,
	                               std::vector<std::vector<GLfloat>> *buffers,
	                               std::vector<int> *counts) const
{
	if (buffers == nullptr || counts == nullptr) {
		return;
	}

	buffers->assign(material_count, {});
	counts->assign(material_count, 0);

	for (const PrimitiveInstance &instance : primitive_instances) {
		if (instance.removed) {
			continue;
		}
		if (instance.material_index < 0 ||
		    static_cast<std::size_t>(instance.material_index) >= material_count) {
			continue;
		}
		(*counts)[instance.material_index] += static_cast<int>(vertex_count_for_instance(instance));
	}

	for (std::size_t material_index = 0; material_index < material_count; ++material_index) {
		(*buffers)[material_index].reserve(
			static_cast<std::size_t>((*counts)[material_index]) * kVertexStride);
	}

	for (const PrimitiveInstance &instance : primitive_instances) {
		if (instance.removed) {
			continue;
		}
		if (instance.material_index < 0 ||
		    static_cast<std::size_t>(instance.material_index) >= material_count) {
			continue;
		}
		append_instance_vertices(instance, vertex_data, &(*buffers)[instance.material_index]);
	}
}

void SceneGenerationContext::buildInstanceBuffer(const PrimitiveInstance &instance,
                                  const GLfloat *vertex_data,
                                  std::vector<GLfloat> *buffer) const
{
	if (buffer == nullptr) {
		return;
	}
	append_instance_vertices(instance, vertex_data, buffer);
}

glm::vec3 SceneGenerationContext::getInstanceCenter(const PrimitiveInstance &instance) const
{
	const PrimitiveBounds bounds = collision_geometry_for_instance(instance).bounds;
	if (bounds.valid) {
		return bounds.center;
	}
	return instance.position;
}

PrimitiveBounds SceneGenerationContext::getInstanceBounds(const PrimitiveInstance &instance) const
{
	return collision_geometry_for_instance(instance).bounds;
}

const CollisionGeometry &SceneGenerationContext::getInstanceCollisionGeometry(const PrimitiveInstance &instance) const
{
	return collision_geometry_for_instance(instance);
}

PrimitiveBounds SceneGenerationContext::getSceneBounds() const
{
	PrimitiveBounds scene_bounds;
	for (const PrimitiveInstance &instance : primitive_instances) {
		if (instance.removed) {
			continue;
		}
		const PrimitiveBounds bounds = collision_geometry_for_instance(instance).bounds;
		if (!bounds.valid) {
			continue;
		}
		if (!scene_bounds.valid) {
			scene_bounds = bounds;
			continue;
		}
		scene_bounds.min = glm::min(scene_bounds.min, bounds.min);
		scene_bounds.max = glm::max(scene_bounds.max, bounds.max);
	}
	if (scene_bounds.valid) {
		scene_bounds.center = (scene_bounds.min + scene_bounds.max) * 0.5f;
		scene_bounds.half_extents = (scene_bounds.max - scene_bounds.min) * 0.5f;
	}
	return scene_bounds;
}

void SceneGenerationContext::PLY(const GLfloat *vertex_data, const std::string &filename)
{
	PLYWriter::writeMesh(filename, buildExportMesh(vertex_data));
}

Mesh SceneGenerationContext::buildExportMesh(const GLfloat *vertex_data) const
{
	Mesh export_mesh;
	if (vertex_data == nullptr) {
		return export_mesh;
	}

	std::vector<GLfloat> buffer;
	buffer.reserve(static_cast<std::size_t>(kPrimitiveVertexCount * kVertexStride));
	for (const PrimitiveInstance &instance : primitive_instances) {
		if (instance.removed) {
			continue;
		}
		buffer.clear();
		append_instance_vertices(instance, vertex_data, &buffer);
		for (int vertex_index = 0;
		     vertex_index + 2 < static_cast<int>(buffer.size() / kVertexStride);
		     vertex_index += 3) {
			const int first_export_vertex = static_cast<int>(export_mesh.vertices.size());
			for (int triangle_vertex = 0; triangle_vertex < 3; ++triangle_vertex) {
				const int source_vertex_index = vertex_index + triangle_vertex;
				const std::size_t offset = static_cast<std::size_t>(source_vertex_index * kVertexStride);
				export_mesh.vertices.emplace_back(buffer[offset],
				                                  buffer[offset + 1],
				                                  buffer[offset + 2]);
			}
			export_mesh.addTriangle(first_export_vertex,
			                        first_export_vertex + 1,
			                        first_export_vertex + 2);
		}
	}
	return export_mesh;
}

void SceneGenerationContext::draw()
{
	for (const PrimitiveInstance &instance : primitive_instances) {
		if (instance.removed) {
			continue;
		}
		(void)instance;
	}
}

const std::vector<std::string> &SceneGenerationContext::getMaterialNames() const
{
	return material_names_;
}

void SceneGenerationContext::genPrimitives()
{
	Cube = new Primitive("Cube", 0, false, false, false);
	CubeX = new Primitive("CubeX", 0, true, false, false);
	CubeY = new Primitive("CubeY", 0, false, true, false);
	CubeZ = new Primitive("CubeZ", 0, false, false, true);
	ProceduralShape = new Primitive("ProceduralShape", 0, false, false, false);
	AxialProfile = new Primitive("AxialProfile", 0, false, false, false);
	Cylinder = new Primitive("Cylinder", 0, false, false, false);
	Sphere = new Primitive("Sphere", 0, false, false, false);
}

SceneGenerationContext::SceneGenerationContext()
	: primitive_geometry_resolver_(std::make_unique<PrimitiveGeometryResolver>()),
	  spatial_scene_construction_state_(
		  std::make_unique<SpatialSceneConstructionState>()),
	  lighting_scene_definition_(std::make_unique<LightingSceneDefinition>()),
	  vehicle_joint_graph_(std::make_unique<VehicleJointGraph>())
{
	primitive_instances.clear();
	material_names_.clear();
	collision_particles_.reserve(kMaxCollisionParticles);
	if (current_scope == nullptr) {
		current_scope = new Scope();
	}
	clearPendingPrimitiveState();
}

bool SceneGenerationContext::addSceneLight(SceneLight light, std::string *diagnostic)
{
	return lighting_scene_definition_ != nullptr &&
	       lighting_scene_definition_->addLight(std::move(light), diagnostic);
}

bool SceneGenerationContext::addLightFixture(
	LightFixtureObject fixture,
	std::string *diagnostic)
{
	return lighting_scene_definition_ != nullptr &&
	       lighting_scene_definition_->addFixture(std::move(fixture), diagnostic);
}

bool SceneGenerationContext::addLightFixtureWithEmitter(
	SceneLight light,
	LightFixtureObject fixture,
	std::string *diagnostic)
{
	return lighting_scene_definition_ != nullptr &&
	       lighting_scene_definition_->addFixtureWithEmitter(
		       std::move(light), std::move(fixture), diagnostic);
}

bool SceneGenerationContext::addLightSwitch(LightSwitch light_switch, std::string *diagnostic)
{
	return lighting_scene_definition_ != nullptr &&
	       lighting_scene_definition_->addSwitch(std::move(light_switch), diagnostic);
}

bool SceneGenerationContext::addLightingCircuit(
	LightingCircuit circuit,
	std::string *diagnostic)
{
	return lighting_scene_definition_ != nullptr &&
	       lighting_scene_definition_->addCircuit(std::move(circuit), diagnostic);
}

const LightingSceneDefinition &SceneGenerationContext::lightingSceneDefinition() const
{
	return *lighting_scene_definition_;
}

bool SceneGenerationContext::addVehicleJoint(
	VehicleJoint joint,
	std::string *diagnostic)
{
	if (vehicle_joint_graph_ == nullptr) {
		if (diagnostic != nullptr) *diagnostic = "Vehicle joint graph is unavailable.";
		return false;
	}
	const VehicleValidationReport validation =
		VehicleKinematicValidationService().validateJoints({joint});
	if (!validation.isValid()) {
		if (diagnostic != nullptr) {
			*diagnostic = validation.issues().front().message();
		}
		return false;
	}
	return vehicle_joint_graph_->addJoint(std::move(joint), diagnostic);
}

const VehicleJointGraph &SceneGenerationContext::vehicleJointGraph() const
{
	return *vehicle_joint_graph_;
}

SceneGenerationContext::~SceneGenerationContext()
{
	delete Cube;
	delete CubeX;
	delete CubeY;
	delete CubeZ;
	delete ProceduralShape;
	delete AxialProfile;
	delete Cylinder;
	delete Sphere;
	if (current_scope != nullptr) {
		delete current_scope;
		current_scope = nullptr;
	}
	while (!scopes.empty()) {
		delete scopes.top();
		scopes.pop();
	}
}

void SceneGenerationContext::newScope()
{
	if (current_scope == nullptr) {
		current_scope = new Scope();
	}
	scopes.push(current_scope);
	const glm::vec3 pos = current_scope->getPosition();
	current_scope = new Scope();
	current_scope->setPosition(pos);
}

void SceneGenerationContext::pushScope()
{
	if (current_scope == nullptr) {
		current_scope = new Scope();
	}
	scopes.push(current_scope);
	current_scope = new Scope(current_scope);
}

Scope *SceneGenerationContext::popScope()
{
	if (scopes.empty()) {
		errorout("Grammar execution error: attempted to pop an empty scope stack.");
		if (current_scope == nullptr) {
			current_scope = new Scope();
		}
		return current_scope;
	}

	if (current_scope != nullptr) {
		delete current_scope;
	}
	current_scope = scopes.top();
	scopes.pop();
	if (current_scope == nullptr) {
		errorout("Grammar execution warning: restored scope was null; a fresh scope was created.");
		current_scope = new Scope();
	}
	return current_scope;
}

Scope *SceneGenerationContext::getCurrentScope()
{
	return current_scope;
}

Mesh &SceneGenerationContext::getScene()
{
	return scene;
}

std::size_t SceneGenerationContext::resolveMaterialSlot(const std::string &material_name)
{
	const std::string canonical_name = canonicalize_material_name(material_name);
	for (std::size_t index = 0; index < material_names_.size(); ++index) {
		if (material_names_[index] == canonical_name) {
			return index;
		}
	}
	material_names_.push_back(canonical_name);
	return material_names_.size() - 1;
}

void SceneGenerationContext::setPendingVelocity(const glm::vec3 &velocity)
{
	pending_velocity_ = velocity;
}

void SceneGenerationContext::setPendingRotationalVelocity(const glm::vec3 &rotational_velocity)
{
	pending_rotational_velocity_ =
		clamp_vector_magnitude(rotational_velocity, kMaxRotationalSpeed);
}

void SceneGenerationContext::setPendingMass(float mass)
{
	pending_mass_ = std::max(mass, kMinimumMass);
}

void SceneGenerationContext::setPendingDensity(float density)
{
	pending_density_ = std::max(density, 0.0f);
	has_pending_density_ = true;
}

void SceneGenerationContext::setGravity(float magnitude, const glm::vec3 &direction)
{
	const float direction_length = glm::length(direction);
	if (direction_length <= kMinDelta || std::fabs(magnitude) <= kMinDelta) {
		gravity_acceleration_ = glm::vec3(0.0f);
		return;
	}

	gravity_acceleration_ = (direction / direction_length) * magnitude;
}

const glm::vec3 &SceneGenerationContext::getGravity() const
{
	return gravity_acceleration_;
}

const std::vector<CollisionParticle> &SceneGenerationContext::getCollisionParticles() const
{
	return collision_particles_;
}

void SceneGenerationContext::emitCollisionParticles(const glm::vec3 &contact_point,
                                     const glm::vec3 &normal,
                                     const glm::vec3 &relative_velocity,
                                     float impulse_magnitude)
{
	const float impact_speed = glm::length(relative_velocity);
	if (impact_speed <= 0.12f && impulse_magnitude <= 0.08f) {
		return;
	}

	vec3 emission_normal = normal;
	const float normal_length2 = glm::dot(emission_normal, emission_normal);
	if (normal_length2 <= (kMinDelta * kMinDelta)) {
		if (impact_speed <= kMinDelta) {
			emission_normal = vec3(0.0f, 1.0f, 0.0f);
		} else {
			emission_normal = relative_velocity / impact_speed;
		}
	} else {
		emission_normal /= std::sqrt(normal_length2);
	}

	const vec3 tangent = orthogonal_unit_vector(emission_normal);
	vec3 bitangent = glm::cross(emission_normal, tangent);
	const float bitangent_length2 = glm::dot(bitangent, bitangent);
	if (bitangent_length2 <= (kMinDelta * kMinDelta)) {
		bitangent = vec3(0.0f, 0.0f, 1.0f);
	} else {
		bitangent /= std::sqrt(bitangent_length2);
	}

	const float intensity = std::clamp(
		1.0f - std::exp(-(impulse_magnitude * 0.045f + impact_speed * 0.42f)),
		0.0f,
		1.0f);
	int particle_count = static_cast<int>(std::round(
		static_cast<float>(kCollisionParticleMinBurst) +
		intensity * static_cast<float>(kCollisionParticleMaxBurst - kCollisionParticleMinBurst)));
	particle_count = std::clamp(particle_count,
	                            kCollisionParticleMinBurst,
	                            kCollisionParticleMaxBurst);
	if (particle_count <= 0) {
		return;
	}

	if (collision_particles_.size() >= kMaxCollisionParticles) {
		return;
	}
	const std::size_t available_slots = kMaxCollisionParticles - collision_particles_.size();
	particle_count = std::min<int>(particle_count, static_cast<int>(available_slots));
	if (particle_count <= 0) {
		return;
	}

	std::uniform_real_distribution<float> unit_dist(0.0f, 1.0f);
	collision_particles_.reserve(kMaxCollisionParticles);
	for (int index = 0; index < particle_count; ++index) {
		const float angle = unit_dist(effects_rng) * glm::two_pi<float>();
		const float radial = std::sqrt(unit_dist(effects_rng));
		vec3 direction = emission_normal * (0.5f + unit_dist(effects_rng) * 0.9f)
			+ tangent * (std::cos(angle) * radial)
			+ bitangent * (std::sin(angle) * radial);
		const float direction_length2 = glm::dot(direction, direction);
		if (direction_length2 <= (kMinDelta * kMinDelta)) {
			direction = emission_normal;
		} else {
			direction /= std::sqrt(direction_length2);
		}

		const float speed_mix = std::clamp((intensity * 0.75f) + (unit_dist(effects_rng) * 0.25f), 0.0f, 1.0f);
		const float speed =
			glm::mix(kCollisionParticleMinSpeed, kCollisionParticleMaxSpeed, speed_mix);
		const float lifetime =
			glm::mix(kCollisionParticleMinLifetime, kCollisionParticleMaxLifetime, unit_dist(effects_rng));
		const float size = glm::mix(0.03f, 0.12f, unit_dist(effects_rng) * (0.45f + intensity * 0.55f));

		CollisionParticle particle;
		particle.position = contact_point + emission_normal * (0.01f + unit_dist(effects_rng) * 0.015f);
		particle.velocity = direction * speed;
		particle.color = glm::vec4(1.0f,
		                           glm::mix(0.72f, 0.95f, unit_dist(effects_rng)),
		                           glm::mix(0.18f, 0.5f, unit_dist(effects_rng)),
		                           1.0f);
		particle.lifetime = lifetime;
		particle.initial_lifetime = lifetime;
		particle.size = size;
		collision_particles_.push_back(particle);
	}
}

void SceneGenerationContext::stepCollisionParticles(float delta_time)
{
	if (delta_time <= 0.0f || collision_particles_.empty()) {
		return;
	}

	const float damping = std::pow(kCollisionParticleDamping, delta_time * 60.0f);
	const vec3 particle_gravity = gravity_acceleration_ * kCollisionParticleGravityScale;
	std::size_t write_index = 0;
	for (std::size_t read_index = 0; read_index < collision_particles_.size(); ++read_index) {
		CollisionParticle particle = collision_particles_[read_index];
		particle.lifetime -= delta_time;
		if (particle.lifetime <= 0.0f) {
			continue;
		}
		particle.velocity += particle_gravity * delta_time;
		particle.velocity *= damping;
		particle.position += particle.velocity * delta_time;
		if (particle.initial_lifetime > 0.0f) {
			const float life_ratio = std::clamp(particle.lifetime / particle.initial_lifetime, 0.0f, 1.0f);
			particle.color.a = life_ratio * life_ratio;
		}
		collision_particles_[write_index++] = particle;
	}
	collision_particles_.resize(write_index);
}

void SceneGenerationContext::clearPendingPrimitiveState()
{
	pending_velocity_ = glm::vec3(0.0f);
	pending_rotational_velocity_ = glm::vec3(0.0f);
	pending_mass_ = kDefaultMass;
	pending_density_ = 0.0f;
	has_pending_density_ = false;
}

void SceneGenerationContext::resetSimulation()
{
	for (PrimitiveInstance &instance : primitive_instances) {
		instance.restoreInitialState();
		invalidate_collision_geometry(&instance);
		capture_previous_render_state(&instance);
	}
	collision_particles_.clear();
	time = 0.0f;
}

void SceneGenerationContext::stepSimulation(float delta_time)
{
	if (delta_time <= 0.0f) {
		return;
	}
	stepCollisionParticles(delta_time);
	if (primitive_instances.empty()) {
		time += delta_time;
		return;
	}

	PrimitiveBounds scene_bounds;
	if (has_scene_bounds_) {
		scene_bounds.min = scene_bounds_min_;
		scene_bounds.max = scene_bounds_max_;
		scene_bounds.center = (scene_bounds.min + scene_bounds.max) * 0.5f;
		scene_bounds.half_extents = (scene_bounds.max - scene_bounds.min) * 0.5f;
		scene_bounds.valid = true;
	}

	std::vector<const CollisionGeometry *> collision_geometries(primitive_instances.size(), nullptr);
	for (PrimitiveInstance &instance : primitive_instances) {
		capture_previous_render_state(&instance);
	}
	for (std::size_t index = 0; index < primitive_instances.size(); ++index) {
		PrimitiveInstance &instance = primitive_instances[index];
		if (!is_dynamic_simulation_active(instance)) {
			continue;
		}
		instance.velocity += gravity_acceleration_for_instance(instance, gravity_acceleration_) * delta_time;
		clamp_rotational_velocity(&instance);
		apply_world_translation(&instance, instance.velocity * delta_time);
		const CollisionGeometry *geometry = &collision_geometry_for_instance(instance);
		if (remove_dynamic_cube_if_outside_scene_bounds(&instance, *geometry, scene_bounds)) {
			collision_geometries[index] = nullptr;
			continue;
		}
		if (stop_simulation_if_outside_bounds(&instance, *geometry)) {
			collision_geometries[index] = nullptr;
			continue;
		}
		if (glm::dot(instance.rotational_velocity, instance.rotational_velocity) > (kMinDelta * kMinDelta)) {
			apply_world_rotation(&instance,
			                    instance.rotational_velocity * delta_time,
			                    geometry->center_of_mass);
			collision_geometries[index] = nullptr;
			continue;
		}
		collision_geometries[index] = geometry;
	}

	std::vector<std::size_t> collidable_indices;
	collidable_indices.reserve(primitive_instances.size());
	for (std::size_t index = 0; index < primitive_instances.size(); ++index) {
		if (!is_collision_candidate(primitive_instances[index])) {
			continue;
		}
		if (collision_geometries[index] == nullptr) {
			collision_geometries[index] = &collision_geometry_for_instance(primitive_instances[index]);
		}
		if (collision_geometries[index] == nullptr) {
			continue;
		}
		if (remove_dynamic_cube_if_outside_scene_bounds(&primitive_instances[index], *collision_geometries[index], scene_bounds)) {
			collision_geometries[index] = nullptr;
			continue;
		}
		if (stop_simulation_if_outside_bounds(&primitive_instances[index], *collision_geometries[index])) {
			collision_geometries[index] = nullptr;
			continue;
		}
		collidable_indices.push_back(index);
	}

	std::vector<bool> had_collision_contact(primitive_instances.size(), false);

	for (int iteration = 0; iteration < 3; ++iteration) {
			const std::vector<BroadPhasePair> candidate_pairs =
				build_broad_phase_pairs(primitive_instances,
				                        collidable_indices,
				                        collision_geometries);
			bool had_collision = false;
			for (const BroadPhasePair &pair : candidate_pairs) {
				const std::size_t i = pair.a;
				const std::size_t j = pair.b;
			if (!is_collision_candidate(primitive_instances[i])) {
				continue;
			}
				if (!is_collision_candidate(primitive_instances[j])) {
					continue;
				}
				if (collision_geometries[i] == nullptr) {
					collision_geometries[i] = &collision_geometry_for_instance(primitive_instances[i]);
				}
				if (collision_geometries[j] == nullptr) {
					collision_geometries[j] = &collision_geometry_for_instance(primitive_instances[j]);
				}
				if (collision_geometries[i] == nullptr || collision_geometries[j] == nullptr) {
					continue;
				}
				if (resolve_collision(this,
				                     &primitive_instances[i],
				                     &primitive_instances[j],
				                     *collision_geometries[i],
				                     *collision_geometries[j])) {
					had_collision = true;
					had_collision_contact[i] = true;
					had_collision_contact[j] = true;
					collision_geometries[i] = &collision_geometry_for_instance(primitive_instances[i]);
					collision_geometries[j] = &collision_geometry_for_instance(primitive_instances[j]);
					remove_dynamic_cube_if_outside_scene_bounds(&primitive_instances[i], *collision_geometries[i], scene_bounds);
					remove_dynamic_cube_if_outside_scene_bounds(&primitive_instances[j], *collision_geometries[j], scene_bounds);
					stop_simulation_if_outside_bounds(&primitive_instances[i], *collision_geometries[i]);
					stop_simulation_if_outside_bounds(&primitive_instances[j], *collision_geometries[j]);
				}
			}
		if (!had_collision) {
			break;
		}
	}

	for (std::size_t index = 0; index < primitive_instances.size(); ++index) {
		PrimitiveInstance &instance = primitive_instances[index];
		if (instance.removed || instance.immovable || !instance.simulation_active) {
			continue;
		}
		if (instance.sleeping) {
			if (exceeds_wake_threshold(instance)) {
				wake_instance(&instance);
			}
			continue;
		}
		if (had_collision_contact[index] && is_below_sleep_threshold(instance)) {
			instance.sleep_timer += delta_time;
			if (instance.sleep_timer >= kSleepDelay) {
				put_instance_to_sleep(&instance);
			}
			continue;
		}
		instance.sleep_timer = 0.0f;
	}

	time += delta_time;
}

float SceneGenerationContext::getTime() const
{
	return time;
}
