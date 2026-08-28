#include "vegetation/service/VineGrowthService.h"

#include "vegetation/model/SurfaceAttachmentSpecification.h"
#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/BranchRadiusConservationService.h"
#include "vegetation/service/SurfaceAttachmentService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr float kDirectionTolerance = 1.0e-7f;

struct MutableBranchNode
{
	std::string identifier;
	glm::vec3 position{0.0f};
	float radius = 0.0f;
	float developmental_age = 0.0f;
	int branch_order = 0;
	BranchNodeState state = BranchNodeState::Active;
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

AxisAlignedBounds expanded_bounds(
	const AxisAlignedBounds &bounds,
	float clearance)
{
	AxisAlignedBounds expanded = bounds;
	const glm::vec3 amount(clearance);
	expanded.min -= amount;
	expanded.max += amount;
	expanded.center = (expanded.min + expanded.max) * 0.5f;
	expanded.half_extents = (expanded.max - expanded.min) * 0.5f;
	return expanded;
}

bool segment_intersects_bounds(
	const glm::vec3 &start,
	const glm::vec3 &end,
	const AxisAlignedBounds &bounds)
{
	const glm::vec3 direction = end - start;
	float minimum_parameter = 0.0f;
	float maximum_parameter = 1.0f;
	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(direction[axis]) <= kDirectionTolerance) {
			if (start[axis] < bounds.min[axis] ||
			    start[axis] > bounds.max[axis]) {
				return false;
			}
			continue;
		}
		float first = (bounds.min[axis] - start[axis]) / direction[axis];
		float second = (bounds.max[axis] - start[axis]) / direction[axis];
		if (first > second) std::swap(first, second);
		minimum_parameter = std::max(minimum_parameter, first);
		maximum_parameter = std::min(maximum_parameter, second);
		if (minimum_parameter > maximum_parameter) return false;
	}
	return true;
}

bool growth_mode_is_valid(VineGrowthMode mode)
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

bool collision_behavior_is_valid(VegetationCollisionBehavior behavior)
{
	switch (behavior) {
	case VegetationCollisionBehavior::Avoid:
	case VegetationCollisionBehavior::Seek:
	case VegetationCollisionBehavior::PermitIntersection:
		return true;
	}
	return false;
}

bool attachment_mode_is_valid(SurfaceAttachmentMode mode)
{
	switch (mode) {
	case SurfaceAttachmentMode::Contact:
	case SurfaceAttachmentMode::Offset:
	case SurfaceAttachmentMode::Twine:
		return true;
	}
	return false;
}

bool growth_mode_requires_target(VineGrowthMode mode)
{
	return mode == VineGrowthMode::WallClimbing ||
	       mode == VineGrowthMode::TrellisClimbing ||
	       mode == VineGrowthMode::Twining ||
	       mode == VineGrowthMode::TendrilClimbing;
}

bool node_can_grow(const BranchNode &node)
{
	return node.state() != BranchNodeState::Dormant &&
	       node.state() != BranchNodeState::Senescent &&
	       node.state() != BranchNodeState::Dead;
}

glm::vec3 normalized_or(const glm::vec3 &direction, const glm::vec3 &fallback)
{
	if (finite(direction) && glm::length(direction) > kDirectionTolerance) {
		return glm::normalize(direction);
	}
	return glm::normalize(fallback);
}

glm::vec3 perpendicular_axis(const glm::vec3 &direction)
{
	const glm::vec3 reference = std::fabs(direction.y) < 0.85f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(direction, reference));
}

glm::vec3 constrain_direction(glm::vec3 direction, VineGrowthMode mode)
{
	if (mode == VineGrowthMode::GroundCreeping) direction.y = 0.0f;
	if (mode == VineGrowthMode::Hanging && direction.y >= -0.05f) {
		direction.y = -std::max(0.25f, std::fabs(direction.y));
	}
	return normalized_or(direction, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool segment_intersects_obstacle(
	const glm::vec3 &start,
	const glm::vec3 &end,
	const VegetationObstacleBoundary &obstacle)
{
	return segment_intersects_bounds(
		start, end, expanded_bounds(obstacle.bounds(), obstacle.clearance()));
}

bool candidate_is_blocked(
	const glm::vec3 &start,
	const glm::vec3 &end,
	const std::vector<VegetationObstacleBoundary> &obstacles)
{
	for (const VegetationObstacleBoundary &obstacle : obstacles) {
		if (segment_intersects_obstacle(start, end, obstacle)) return true;
	}
	return false;
}

bool resolve_collision_aware_direction(
	const glm::vec3 &start,
	const glm::vec3 &desired_direction,
	float step_length,
	VineGrowthMode growth_mode,
	VegetationCollisionBehavior collision_behavior,
	const std::vector<VegetationObstacleBoundary> &obstacles,
	std::size_t *avoided_collision_count,
	glm::vec3 *resolved_direction)
{
	const glm::vec3 constrained_desired =
		constrain_direction(desired_direction, growth_mode);
	if (collision_behavior ==
	    VegetationCollisionBehavior::PermitIntersection) {
		*resolved_direction = constrained_desired;
		return true;
	}
	const glm::vec3 first_lateral = perpendicular_axis(constrained_desired);
	const glm::vec3 second_lateral = normalized_or(
		glm::cross(constrained_desired, first_lateral),
		glm::vec3(0.0f, 0.0f, 1.0f));
	const std::array<glm::vec3, 7> directions = {
		constrained_desired,
		constrained_desired + first_lateral * 0.80f,
		constrained_desired - first_lateral * 0.80f,
		constrained_desired + second_lateral * 0.80f,
		constrained_desired - second_lateral * 0.80f,
		constrained_desired + first_lateral * 0.55f + second_lateral * 0.55f,
		constrained_desired - first_lateral * 0.55f - second_lateral * 0.55f,
	};
	const bool direct_candidate_blocked = candidate_is_blocked(
		start, start + constrained_desired * step_length, obstacles);
	for (const glm::vec3 &direction : directions) {
		const glm::vec3 candidate_direction =
			constrain_direction(direction, growth_mode);
		if (candidate_is_blocked(
				start, start + candidate_direction * step_length, obstacles)) {
			continue;
		}
		if (direct_candidate_blocked && avoided_collision_count != nullptr) {
			++(*avoided_collision_count);
		}
		*resolved_direction = candidate_direction;
		return true;
	}
	return false;
}

glm::vec3 free_growth_direction(
	VineGrowthMode mode,
	const glm::vec3 &current_direction,
	const glm::vec3 &preferred_direction)
{
	if (mode == VineGrowthMode::Hanging) {
		return glm::vec3(0.0f, -1.0f, 0.0f);
	}
	if (mode == VineGrowthMode::GroundCreeping) {
		return constrain_direction(
			current_direction * 0.6f + preferred_direction * 0.4f, mode);
	}
	return normalized_or(
		current_direction * 0.6f + preferred_direction * 0.4f,
		preferred_direction);
}

glm::vec3 seeking_direction(
	const glm::vec3 &position,
	const glm::vec3 &current_direction,
	const glm::vec3 &preferred_direction,
	const SurfaceAttachmentResolution &nearest_attachment,
	VineGrowthMode mode)
{
	const glm::vec3 toward_surface = normalized_or(
		nearest_attachment.attachedPoint() - position,
		-nearest_attachment.surfaceNormal());
	return constrain_direction(
		toward_surface * 0.70f + current_direction * 0.20f +
			preferred_direction * 0.10f,
		mode);
}

glm::vec3 deterministic_surface_tangent(
	const glm::vec3 &surface_normal,
	const glm::vec3 &preferred_direction,
	VineGrowthMode growth_mode,
	std::size_t segment_index)
{
	glm::vec3 base_direction = preferred_direction;
	if (growth_mode == VineGrowthMode::Hanging) {
		base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
	}
	glm::vec3 tangent =
		base_direction - surface_normal * glm::dot(base_direction, surface_normal);
	if (glm::length(tangent) <= kDirectionTolerance) {
		const glm::vec3 vertical(0.0f, 1.0f, 0.0f);
		tangent = vertical - surface_normal * glm::dot(vertical, surface_normal);
	}
	if (glm::length(tangent) <= kDirectionTolerance) {
		tangent = perpendicular_axis(surface_normal);
	}
	tangent = glm::normalize(tangent);
	if (growth_mode == VineGrowthMode::Twining ||
	    growth_mode == VineGrowthMode::TendrilClimbing) {
		const glm::vec3 lateral = normalized_or(
			glm::cross(surface_normal, tangent), perpendicular_axis(tangent));
		const float direction_sign = segment_index % 2u == 0u ? 1.0f : -1.0f;
		const float amplitude = growth_mode == VineGrowthMode::Twining
			? 0.45f
			: 0.25f;
		tangent = glm::normalize(
			tangent + lateral * direction_sign * amplitude);
	}
	return constrain_direction(tangent, growth_mode);
}

std::string next_unique_identifier(
	const std::string &prefix,
	std::size_t *counter,
	std::unordered_set<std::string> *identifiers)
{
	for (;;) {
		const std::string identifier = prefix + std::to_string((*counter)++);
		if (identifiers->insert(identifier).second) return identifier;
	}
}

std::vector<MutableBranchNode> mutable_nodes(const BranchGraph &graph)
{
	std::vector<MutableBranchNode> result;
	result.reserve(graph.nodes().size());
	for (const BranchNode &node : graph.nodes()) {
		result.push_back(MutableBranchNode{
			node.identifier(), node.position(), node.radius(),
			node.developmentalAge(), node.branchOrder(), node.state()});
	}
	return result;
}

std::vector<BranchNode> immutable_nodes(
	const std::vector<MutableBranchNode> &nodes)
{
	std::vector<BranchNode> result;
	result.reserve(nodes.size());
	for (const MutableBranchNode &node : nodes) {
		result.emplace_back(
			node.identifier, node.position, node.radius,
			node.developmental_age, node.branch_order, node.state);
	}
	return result;
}

void reconcile_radii(
	std::vector<MutableBranchNode> *nodes,
	const std::vector<BranchSegment> &segments,
	const std::string &root_node_identifier,
	float minimum_radius,
	float exponent_gamma)
{
	std::unordered_map<std::string, std::size_t> node_indices;
	for (std::size_t index = 0; index < nodes->size(); ++index) {
		node_indices.emplace((*nodes)[index].identifier, index);
	}
	std::unordered_map<std::string, std::vector<std::string>> children;
	for (const BranchSegment &segment : segments) {
		children[segment.parentNodeIdentifier()].push_back(
			segment.childNodeIdentifier());
	}
	const BranchGraph preliminary(
		root_node_identifier, immutable_nodes(*nodes), segments);
	const std::vector<std::string> traversal =
		BranchGraphValidationService().deterministicTraversal(preliminary);
	for (auto iterator = traversal.rbegin(); iterator != traversal.rend(); ++iterator) {
		MutableBranchNode &node = (*nodes)[node_indices.at(*iterator)];
		node.radius = std::max(node.radius, minimum_radius);
		const auto found_children = children.find(*iterator);
		if (found_children == children.end() || found_children->second.empty()) {
			continue;
		}
		if (found_children->second.size() == 1u) {
			node.radius = std::max(
				node.radius,
				(*nodes)[node_indices.at(found_children->second.front())].radius);
			continue;
		}
		float child_measure = 0.0f;
		for (const std::string &child_identifier : found_children->second) {
			child_measure += std::pow(
				(*nodes)[node_indices.at(child_identifier)].radius,
				exponent_gamma);
		}
		node.radius = std::pow(child_measure, 1.0f / exponent_gamma);
	}
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
	std::uint64_t source_graph_hash,
	std::uint64_t resolved_graph_hash,
	const VineGrowthSpecification &specification,
	const std::optional<VegetationSurfaceTarget> &target,
	const std::vector<VegetationObstacleBoundary> &obstacles,
	const std::vector<SurfaceAttachmentPoint> &attachment_points,
	std::size_t added_segment_count,
	std::size_t avoided_collision_count,
	VinePathState path_state)
{
	std::uint64_t value = 1469598103934665603ull;
	hash_bytes(&value, &source_graph_hash, sizeof(source_graph_hash));
	hash_bytes(&value, &resolved_graph_hash, sizeof(resolved_graph_hash));
	const int growth_mode = static_cast<int>(specification.growthMode());
	const int collision_behavior =
		static_cast<int>(specification.collisionBehavior());
	const int attachment_mode =
		static_cast<int>(specification.attachmentMode());
	hash_bytes(&value, &growth_mode, sizeof(growth_mode));
	hash_bytes(&value, &collision_behavior, sizeof(collision_behavior));
	hash_bytes(&value, &attachment_mode, sizeof(attachment_mode));
	for (const float scalar : {
		 specification.stepLength(), specification.maximumSeekDistance(),
		 specification.attachmentDistance(),
		 specification.attachmentTolerance(), specification.radiusDecay(),
		 specification.minimumRadius(),
		 specification.radiusConservationExponent()}) {
		hash_bytes(&value, &scalar, sizeof(scalar));
	}
	const std::size_t maximum_segments = specification.maximumSegments();
	hash_bytes(&value, &maximum_segments, sizeof(maximum_segments));
	hash_vector(&value, specification.preferredDirection());
	const bool has_target = target.has_value();
	hash_bytes(&value, &has_target, sizeof(has_target));
	if (target.has_value()) {
		hash_string(&value, target->identifier());
		const int target_kind = static_cast<int>(target->geometryKind());
		hash_bytes(&value, &target_kind, sizeof(target_kind));
		hash_vector(&value, target->bounds().min);
		hash_vector(&value, target->bounds().max);
	}
	for (const VegetationObstacleBoundary &obstacle : obstacles) {
		hash_string(&value, obstacle.identifier());
		hash_vector(&value, obstacle.bounds().min);
		hash_vector(&value, obstacle.bounds().max);
		const float clearance = obstacle.clearance();
		hash_bytes(&value, &clearance, sizeof(clearance));
	}
	for (const SurfaceAttachmentPoint &attachment : attachment_points) {
		hash_string(&value, attachment.identifier());
		hash_string(&value, attachment.hostNodeIdentifier());
		hash_string(&value, attachment.targetIdentifier());
		hash_vector(&value, attachment.position());
		hash_vector(&value, attachment.normal());
		const float distance = attachment.distance();
		hash_bytes(&value, &distance, sizeof(distance));
	}
	hash_bytes(&value, &added_segment_count, sizeof(added_segment_count));
	hash_bytes(&value, &avoided_collision_count, sizeof(avoided_collision_count));
	const int state = static_cast<int>(path_state);
	hash_bytes(&value, &state, sizeof(state));
	return value;
}

} // namespace

VineGrowthResult VineGrowthService::resolve(
	const VineGrowthRequest &request) const
{
	const VineGrowthSpecification &specification = request.specification();
	if (!growth_mode_is_valid(specification.growthMode()) ||
	    !collision_behavior_is_valid(specification.collisionBehavior()) ||
	    !attachment_mode_is_valid(specification.attachmentMode())) {
		return VineGrowthResult::failed(
			"Vine growth contains an unsupported mode.");
	}
	if (!std::isfinite(specification.stepLength()) ||
	    specification.stepLength() <= 0.0f ||
	    specification.maximumSegments() == 0u ||
	    specification.maximumSegments() >
		    complexity_limits_.maximumGrowthIterations() ||
	    !std::isfinite(specification.maximumSeekDistance()) ||
	    specification.maximumSeekDistance() < 0.0f ||
	    !std::isfinite(specification.attachmentDistance()) ||
	    specification.attachmentDistance() < 0.0f ||
	    !std::isfinite(specification.attachmentTolerance()) ||
	    specification.attachmentTolerance() <= 0.0f ||
	    !std::isfinite(specification.radiusDecay()) ||
	    specification.radiusDecay() <= 0.0f ||
	    specification.radiusDecay() > 1.0f ||
	    !std::isfinite(specification.minimumRadius()) ||
	    specification.minimumRadius() <= 0.0f ||
	    !std::isfinite(specification.radiusConservationExponent()) ||
	    specification.radiusConservationExponent() <= 0.0f ||
	    !finite(specification.preferredDirection()) ||
	    glm::length(specification.preferredDirection()) <= kDirectionTolerance) {
		return VineGrowthResult::failed(
			"Vine growth requires finite positive limits, radii, tolerance, "
			"radius conservation, and direction values.");
	}
	const bool target_required =
		growth_mode_requires_target(specification.growthMode()) ||
		specification.collisionBehavior() == VegetationCollisionBehavior::Seek;
	if (target_required && !request.target().has_value()) {
		return VineGrowthResult::failed(
			"Vine growth mode requires a surface target.");
	}
	if (target_required && specification.maximumSeekDistance() <= 0.0f) {
		return VineGrowthResult::failed(
			"Vine growth seeking requires a positive maximum seek distance.");
	}

	std::optional<SurfaceAttachmentSpecification> attachment_specification;
	const SurfaceAttachmentService attachment_service;
	if (request.target().has_value()) {
		attachment_specification.emplace(
			*request.target(), specification.attachmentDistance(),
			specification.attachmentTolerance(), specification.attachmentMode());
		std::string diagnostic;
		if (!attachment_service.validate(
				*attachment_specification, &diagnostic)) {
			return VineGrowthResult::failed(std::move(diagnostic));
		}
	}

	std::unordered_set<std::string> obstacle_identifiers;
	for (const VegetationObstacleBoundary &obstacle : request.obstacles()) {
		if (obstacle.identifier().empty() ||
		    !obstacle_identifiers.insert(obstacle.identifier()).second ||
		    !finite_bounds(obstacle.bounds()) ||
		    !std::isfinite(obstacle.clearance()) ||
		    obstacle.clearance() < 0.0f) {
			return VineGrowthResult::failed(
				"Vine growth obstacle boundaries must be unique, finite, and valid.");
		}
		if (request.target().has_value() &&
		    obstacle.identifier() == request.target()->identifier()) {
			return VineGrowthResult::failed(
				"Vine growth target cannot also be an avoidance obstacle.");
		}
	}

	const BranchGraphValidationService validation_service(complexity_limits_);
	const BranchGraphValidationReport source_validation =
		validation_service.validate(request.sourceGraph());
	if (!source_validation.succeeded()) {
		return VineGrowthResult::failed(
			"Vine growth source graph is invalid: " +
			source_validation.issues().front().diagnostic());
	}
	if (request.sourceGraph().nodes().size() +
		    specification.maximumSegments() >
		    complexity_limits_.maximumBranchNodes() ||
	    request.sourceGraph().segments().size() +
		    specification.maximumSegments() >
		    complexity_limits_.maximumBranchSegments()) {
		return VineGrowthResult::failed(
			"Vine growth request exceeds branch graph complexity ceilings.");
	}

	std::vector<const VegetationGrowthTip *> active_tips;
	for (const VegetationGrowthTip &tip : request.sourceGraph().growthTips()) {
		if (tip.isActive()) active_tips.push_back(&tip);
	}
	std::sort(
		active_tips.begin(), active_tips.end(),
		[](const VegetationGrowthTip *first,
		   const VegetationGrowthTip *second) {
			return first->identifier() < second->identifier();
		});
	if (active_tips.empty()) {
		return VineGrowthResult::failed(
			"Vine growth requires at least one active growth tip.");
	}
	const VegetationGrowthTip &selected_tip = *active_tips.front();

	std::vector<MutableBranchNode> nodes = mutable_nodes(request.sourceGraph());
	std::vector<BranchSegment> segments = request.sourceGraph().segments();
	nodes.reserve(nodes.size() + specification.maximumSegments());
	segments.reserve(segments.size() + specification.maximumSegments());
	std::unordered_map<std::string, std::size_t> node_indices;
	std::unordered_map<std::string, std::size_t> outgoing_count;
	std::unordered_set<std::string> node_identifiers;
	std::unordered_set<std::string> segment_identifiers;
	std::unordered_set<std::string> tip_identifiers;
	for (std::size_t index = 0; index < nodes.size(); ++index) {
		node_indices.emplace(nodes[index].identifier, index);
		node_identifiers.insert(nodes[index].identifier);
	}
	for (const BranchSegment &segment : segments) {
		segment_identifiers.insert(segment.identifier());
		++outgoing_count[segment.parentNodeIdentifier()];
	}
	for (const VegetationGrowthTip &tip : request.sourceGraph().growthTips()) {
		tip_identifiers.insert(tip.identifier());
	}
	const auto selected_node = node_indices.find(selected_tip.hostNodeIdentifier());
	if (selected_node == node_indices.end() ||
	    !node_can_grow(request.sourceGraph().nodes()[selected_node->second])) {
		return VineGrowthResult::failed(
			"Vine growth selected tip is not hosted by a growable node.");
	}
	for (const VegetationObstacleBoundary &obstacle : request.obstacles()) {
		if (point_inside_bounds(
				nodes[selected_node->second].position,
				expanded_bounds(obstacle.bounds(), obstacle.clearance()))) {
			return VineGrowthResult::failed(
				"Vine growth cannot begin inside an avoidance obstacle.");
		}
	}

	std::size_t current_node_index = selected_node->second;
	glm::vec3 current_direction = normalized_or(
		selected_tip.direction(), specification.preferredDirection());
	const glm::vec3 preferred_direction =
		glm::normalize(specification.preferredDirection());
	VinePathState path_state = target_required
		? VinePathState::Seeking
		: VinePathState::Free;
	std::vector<SurfaceAttachmentPoint> attachment_points;
	attachment_points.reserve(specification.maximumSegments());
	std::size_t avoided_collision_count = 0;
	std::size_t node_counter = 1;
	std::size_t segment_counter = 1;
	std::size_t tip_counter = 1;
	std::size_t attachment_counter = 1;
	std::size_t added_segment_count = 0;

	if (attachment_specification.has_value() && target_required) {
		const SurfaceAttachmentResolution nearest =
			attachment_service.attachNearest(
				nodes[current_node_index].position, *attachment_specification);
		if (!nearest.succeeded()) {
			return VineGrowthResult::failed(nearest.diagnostic());
		}
		if (nearest.sourceDistance() > specification.maximumSeekDistance()) {
			return VineGrowthResult::failed(
				"Vine growth target is outside the maximum seek distance.");
		}
		if (nearest.sourceDistance() <=
		    specification.attachmentDistance() +
			    specification.attachmentTolerance()) {
			path_state = VinePathState::Attached;
		}
	}

	for (std::size_t segment_index = 0;
	     segment_index < specification.maximumSegments();
	     ++segment_index) {
		const MutableBranchNode current_node = nodes[current_node_index];
		glm::vec3 desired_direction;
		SurfaceAttachmentResolution nearest_attachment =
			SurfaceAttachmentResolution::failed("No attachment query performed.");
		if (path_state == VinePathState::Attached) {
			nearest_attachment = attachment_service.attachNearest(
				current_node.position, *attachment_specification);
			if (!nearest_attachment.succeeded()) {
				return VineGrowthResult::failed(nearest_attachment.diagnostic());
			}
			desired_direction = deterministic_surface_tangent(
				nearest_attachment.surfaceNormal(), preferred_direction,
				specification.growthMode(), segment_index);
		}
		else if (target_required) {
			nearest_attachment = attachment_service.attachNearest(
				current_node.position, *attachment_specification);
			if (!nearest_attachment.succeeded()) {
				return VineGrowthResult::failed(nearest_attachment.diagnostic());
			}
			if (nearest_attachment.sourceDistance() >
			    specification.maximumSeekDistance()) {
				path_state = VinePathState::Blocked;
				break;
			}
			if (nearest_attachment.sourceDistance() <=
			    specification.attachmentDistance() +
				    specification.attachmentTolerance()) {
				path_state = VinePathState::Attached;
				desired_direction = deterministic_surface_tangent(
					nearest_attachment.surfaceNormal(), preferred_direction,
					specification.growthMode(), segment_index);
			}
			else {
				desired_direction = seeking_direction(
					current_node.position, current_direction, preferred_direction,
					nearest_attachment, specification.growthMode());
			}
		}
		else {
			desired_direction = free_growth_direction(
				specification.growthMode(), current_direction,
				preferred_direction);
		}

		glm::vec3 resolved_direction;
		if (!resolve_collision_aware_direction(
				current_node.position, desired_direction,
				specification.stepLength(), specification.growthMode(),
				specification.collisionBehavior(), request.obstacles(),
				&avoided_collision_count, &resolved_direction)) {
			path_state = VinePathState::Blocked;
			break;
		}

		glm::vec3 resolved_position =
			current_node.position +
			resolved_direction * specification.stepLength();
		std::optional<SurfaceAttachmentResolution> resolved_attachment;
		if (path_state == VinePathState::Attached) {
			const SurfaceAttachmentResolution projected =
				attachment_service.attachNearest(
					resolved_position, *attachment_specification);
			if (!projected.succeeded()) {
				return VineGrowthResult::failed(projected.diagnostic());
			}
			resolved_position = projected.attachedPoint();
			resolved_attachment = projected;
		}
		else if (target_required) {
			const SurfaceAttachmentResolution contact =
				attachment_service.firstContact(
					current_node.position, resolved_position,
					*attachment_specification);
			if (contact.succeeded()) {
				resolved_position = contact.attachedPoint();
				resolved_attachment = contact;
				path_state = VinePathState::Attached;
			}
		}
		if (glm::distance(current_node.position, resolved_position) <=
		    kDirectionTolerance) {
			path_state = VinePathState::Blocked;
			break;
		}

		const std::string node_identifier = next_unique_identifier(
			"vine_node_", &node_counter, &node_identifiers);
		const std::string segment_identifier = next_unique_identifier(
			"vine_segment_", &segment_counter, &segment_identifiers);
		const bool continuation = outgoing_count[current_node.identifier] == 0u;
		const int branch_order = current_node.branch_order +
			(continuation ? 0 : 1);
		nodes.push_back(MutableBranchNode{
			node_identifier,
			resolved_position,
			std::max(
				current_node.radius * specification.radiusDecay(),
				specification.minimumRadius()),
			current_node.developmental_age + 1.0f,
			branch_order,
			BranchNodeState::Active});
		segments.emplace_back(
			segment_identifier, current_node.identifier, node_identifier,
			continuation
				? BranchSegmentKind::Continuation
				: BranchSegmentKind::LateralBranch);
		++outgoing_count[current_node.identifier];
		current_node_index = nodes.size() - 1u;
		current_direction = normalized_or(
			resolved_position - current_node.position, resolved_direction);
		++added_segment_count;

		if (resolved_attachment.has_value()) {
			const std::string attachment_identifier =
				"vine_attachment_" + std::to_string(attachment_counter++);
			attachment_points.emplace_back(
				attachment_identifier, node_identifier,
				request.target()->identifier(),
				resolved_attachment->attachedPoint(),
				resolved_attachment->surfaceNormal(),
				glm::distance(
					resolved_attachment->surfacePoint(),
					resolved_attachment->attachedPoint()));
		}
	}

	reconcile_radii(
		&nodes, segments, request.sourceGraph().rootNodeIdentifier(),
		specification.minimumRadius(),
		specification.radiusConservationExponent());

	std::vector<VegetationGrowthTip> growth_tips;
	growth_tips.reserve(request.sourceGraph().growthTips().size() + 1u);
	for (const VegetationGrowthTip &tip : request.sourceGraph().growthTips()) {
		const bool selected = tip.identifier() == selected_tip.identifier();
		growth_tips.emplace_back(
			tip.identifier(), tip.hostNodeIdentifier(), tip.direction(),
			selected ? false : tip.isActive());
	}
	if (added_segment_count > 0u && path_state != VinePathState::Blocked) {
		const MutableBranchNode &terminal_node = nodes[current_node_index];
		growth_tips.emplace_back(
			next_unique_identifier(
				"vine_tip_", &tip_counter, &tip_identifiers),
			terminal_node.identifier, current_direction, true);
	}
	if (growth_tips.size() > complexity_limits_.maximumGrowthTips()) {
		return VineGrowthResult::failed(
			"Vine growth exceeded the growth-tip complexity ceiling.");
	}

	BranchGraph resolved_graph(
		request.sourceGraph().rootNodeIdentifier(), immutable_nodes(nodes),
		std::move(segments), request.sourceGraph().buds(),
		request.sourceGraph().organAttachments(), std::move(growth_tips));
	const BranchGraphValidationReport resolved_validation =
		validation_service.validate(resolved_graph);
	if (!resolved_validation.succeeded()) {
		return VineGrowthResult::failed(
			"Vine growth produced an invalid graph: " +
			resolved_validation.issues().front().diagnostic());
	}
	const BranchGraphValidationReport radius_validation =
		BranchRadiusConservationService().validate(
			resolved_graph, specification.radiusConservationExponent(), 0.0001f);
	if (!radius_validation.succeeded()) {
		return VineGrowthResult::failed(
			"Vine growth could not reconcile branch radii: " +
			radius_validation.issues().front().diagnostic());
	}

	const BranchGraphDeterministicHashService hash_service;
	const std::uint64_t source_graph_hash =
		hash_service.hash(request.sourceGraph());
	const std::uint64_t resolved_graph_hash = hash_service.hash(resolved_graph);
	const std::uint64_t resolved_evidence_hash = evidence_hash(
		source_graph_hash, resolved_graph_hash, specification,
		request.target(), request.obstacles(), attachment_points,
		added_segment_count, avoided_collision_count, path_state);
	const std::size_t attachment_count = attachment_points.size();
	return VineGrowthResult::succeeded(VineGrowthSnapshot(
		VinePath(
			std::move(resolved_graph), std::move(attachment_points),
			path_state),
		VineGrowthResolutionEvidence(
			source_graph_hash, resolved_graph_hash, resolved_evidence_hash,
			added_segment_count, attachment_count,
			avoided_collision_count, path_state)));
}
