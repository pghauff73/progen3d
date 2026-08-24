#include "vegetation/service/SpaceColonizationService.h"

#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/BranchRadiusConservationService.h"
#include "vegetation/service/CrownAttractionPointSamplingService.h"
#include "vegetation/service/CrownVolumeContainmentService.h"
#include "vegetation/service/SpaceColonizationSpecificationValidationService.h"

#include <algorithm>
#include <array>
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

struct MutableBranchNode
{
	std::string identifier;
	glm::vec3 position{0.0f};
	float radius = 0.0f;
	float developmental_age = 0.0f;
	int branch_order = 0;
	BranchNodeState state = BranchNodeState::Active;
};

struct SpatialCellKey
{
	int x = 0;
	int y = 0;
	int z = 0;

	bool operator==(const SpatialCellKey &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct SpatialCellKeyHash
{
	std::size_t operator()(const SpatialCellKey &key) const
	{
		std::size_t value = static_cast<std::size_t>(key.x) * 73856093u;
		value ^= static_cast<std::size_t>(key.y) * 19349663u;
		value ^= static_cast<std::size_t>(key.z) * 83492791u;
		return value;
	}
};

using NodeSpatialGrid = std::unordered_map<
	SpatialCellKey,
	std::vector<std::size_t>,
	SpatialCellKeyHash>;

struct AttractionAssignment
{
	glm::vec3 direction_sum{0.0f};
	std::size_t count = 0;
};

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

SpatialCellKey cell_for(const glm::vec3 &point, float cell_size)
{
	return SpatialCellKey{
		static_cast<int>(std::floor(point.x / cell_size)),
		static_cast<int>(std::floor(point.y / cell_size)),
		static_cast<int>(std::floor(point.z / cell_size))};
}

NodeSpatialGrid build_spatial_grid(
	const std::vector<MutableBranchNode> &nodes,
	float cell_size)
{
	NodeSpatialGrid grid;
	for (std::size_t node_index = 0; node_index < nodes.size(); ++node_index) {
		grid[cell_for(nodes[node_index].position, cell_size)].push_back(node_index);
	}
	return grid;
}

std::vector<std::size_t> neighboring_nodes(
	const NodeSpatialGrid &grid,
	const glm::vec3 &point,
	float cell_size)
{
	const SpatialCellKey center = cell_for(point, cell_size);
	std::vector<std::size_t> result;
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			for (int z_offset = -1; z_offset <= 1; ++z_offset) {
				const auto found = grid.find(SpatialCellKey{
					center.x + x_offset,
					center.y + y_offset,
					center.z + z_offset});
				if (found != grid.end()) {
					result.insert(
						result.end(), found->second.begin(), found->second.end());
				}
			}
		}
	}
	return result;
}

bool node_can_grow(const MutableBranchNode &node)
{
	return node.state != BranchNodeState::Dormant &&
	       node.state != BranchNodeState::Senescent &&
	       node.state != BranchNodeState::Dead;
}

AxisAlignedBounds expanded_bounds(
	const AxisAlignedBounds &bounds,
	float clearance)
{
	AxisAlignedBounds expanded = bounds;
	const glm::vec3 expansion(clearance);
	expanded.min -= expansion;
	expanded.max += expansion;
	expanded.center = (expanded.min + expanded.max) * 0.5f;
	expanded.half_extents = (expanded.max - expanded.min) * 0.5f;
	return expanded;
}

bool point_inside_bounds(
	const glm::vec3 &point,
	const AxisAlignedBounds &bounds)
{
	return point.x >= bounds.min.x && point.x <= bounds.max.x &&
	       point.y >= bounds.min.y && point.y <= bounds.max.y &&
	       point.z >= bounds.min.z && point.z <= bounds.max.z;
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
		if (std::fabs(direction[axis]) <= 1.0e-8f) {
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

bool segment_intersects_obstacle(
	const glm::vec3 &start,
	const glm::vec3 &end,
	const VegetationObstacleBoundary &obstacle)
{
	return segment_intersects_bounds(
		start, end, expanded_bounds(obstacle.bounds(), obstacle.clearance()));
}

glm::vec3 perpendicular_axis(const glm::vec3 &direction)
{
	const glm::vec3 reference = std::fabs(direction.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(direction, reference));
}

bool too_close_to_nodes(
	const glm::vec3 &candidate,
	const std::vector<MutableBranchNode> &nodes,
	const std::vector<MutableBranchNode> &new_nodes,
	float minimum_spacing)
{
	for (const MutableBranchNode &node : nodes) {
		if (glm::distance(candidate, node.position) < minimum_spacing) return true;
	}
	for (const MutableBranchNode &node : new_nodes) {
		if (glm::distance(candidate, node.position) < minimum_spacing) return true;
	}
	return false;
}

bool candidate_position(
	const MutableBranchNode &parent,
	const glm::vec3 &desired_direction,
	float step_length,
	const CrownVolumeSpecification &crown_volume,
	const std::vector<VegetationObstacleBoundary> &obstacles,
	const std::vector<MutableBranchNode> &nodes,
	const std::vector<MutableBranchNode> &new_nodes,
	std::size_t *obstacle_rejected_candidate_count,
	glm::vec3 *resolved_position,
	glm::vec3 *resolved_direction)
{
	const CrownVolumeContainmentService containment_service;
	const glm::vec3 first_lateral = perpendicular_axis(desired_direction);
	const glm::vec3 second_lateral = glm::normalize(
		glm::cross(desired_direction, first_lateral));
	const std::array<glm::vec3, 5> directions = {
		desired_direction,
		glm::normalize(desired_direction + first_lateral * 0.75f),
		glm::normalize(desired_direction - first_lateral * 0.75f),
		glm::normalize(desired_direction + second_lateral * 0.75f),
		glm::normalize(desired_direction - second_lateral * 0.75f)};
	for (const glm::vec3 &direction : directions) {
		const glm::vec3 candidate = parent.position + direction * step_length;
		if (!containment_service.contains(crown_volume, candidate)) continue;
		if (too_close_to_nodes(
				candidate, nodes, new_nodes, step_length * 0.20f)) {
			continue;
		}
		bool blocked = false;
		for (const VegetationObstacleBoundary &obstacle : obstacles) {
			if (segment_intersects_obstacle(
					parent.position, candidate, obstacle)) {
				blocked = true;
				++(*obstacle_rejected_candidate_count);
				break;
			}
		}
		if (blocked) continue;
		*resolved_position = candidate;
		*resolved_direction = direction;
		return true;
	}
	return false;
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
	BranchGraph preliminary(
		root_node_identifier, immutable_nodes(*nodes), segments);
	std::vector<std::string> traversal =
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

std::uint64_t resolution_evidence_hash(
	std::uint64_t source_graph_hash,
	std::uint64_t resolved_graph_hash,
	std::uint64_t deterministic_seed,
	std::size_t attraction_point_count,
	std::size_t remaining_attraction_point_count,
	std::size_t completed_iterations,
	std::size_t added_node_count,
	std::size_t obstacle_rejected_candidate_count)
{
	std::uint64_t value = 1469598103934665603ull;
	hash_bytes(&value, &source_graph_hash, sizeof(source_graph_hash));
	hash_bytes(&value, &resolved_graph_hash, sizeof(resolved_graph_hash));
	hash_bytes(&value, &deterministic_seed, sizeof(deterministic_seed));
	hash_bytes(&value, &attraction_point_count, sizeof(attraction_point_count));
	hash_bytes(
		&value, &remaining_attraction_point_count,
		sizeof(remaining_attraction_point_count));
	hash_bytes(&value, &completed_iterations, sizeof(completed_iterations));
	hash_bytes(&value, &added_node_count, sizeof(added_node_count));
	hash_bytes(
		&value, &obstacle_rejected_candidate_count,
		sizeof(obstacle_rejected_candidate_count));
	return value;
}

} // namespace

SpaceColonizationResult SpaceColonizationService::resolve(
	const SpaceColonizationRequest &request) const
{
	const SpaceColonizationSpecification &specification = request.specification();
	std::string specification_diagnostic;
	if (!SpaceColonizationSpecificationValidationService(complexity_limits_).validate(
			specification, &specification_diagnostic)) {
		return SpaceColonizationResult::failed(specification_diagnostic);
	}

	const BranchGraphValidationService validation_service(complexity_limits_);
	const BranchGraphValidationReport source_validation =
		validation_service.validate(request.sourceGraph());
	if (!source_validation.succeeded()) {
		return SpaceColonizationResult::failed(
			"Space colonization source graph is invalid: " +
			source_validation.issues().front().diagnostic());
	}

	const CrownVolumeContainmentService containment_service;
	std::string crown_diagnostic;
	if (!containment_service.validate(request.crownVolume(), &crown_diagnostic)) {
		return SpaceColonizationResult::failed(crown_diagnostic);
	}
	for (const VegetationObstacleBoundary &obstacle : request.obstacles()) {
		if (obstacle.identifier().empty() || !finite_bounds(obstacle.bounds()) ||
		    !std::isfinite(obstacle.clearance()) || obstacle.clearance() < 0.0f) {
			return SpaceColonizationResult::failed(
				"Vegetation obstacles require identifiers, valid finite bounds, and "
				"non-negative clearance.");
		}
		const AxisAlignedBounds expanded =
			expanded_bounds(obstacle.bounds(), obstacle.clearance());
		for (const BranchNode &source_node : request.sourceGraph().nodes()) {
			if (point_inside_bounds(source_node.position(), expanded)) {
				return SpaceColonizationResult::failed(
					"Space colonization source graph begins inside obstacle '" +
					obstacle.identifier() + "'.");
			}
		}
	}

	const CrownAttractionPointSamplingResult sampling =
		CrownAttractionPointSamplingService().sample(
			request.crownVolume(), specification.attractionPointCount(),
			request.deterministicSeed());
	if (!sampling.succeeded()) {
		return SpaceColonizationResult::failed(sampling.diagnostic());
	}
	std::vector<glm::vec3> attraction_points = sampling.points();

	std::vector<MutableBranchNode> nodes;
	nodes.reserve(std::min(
		complexity_limits_.maximumBranchNodes(),
		request.sourceGraph().nodes().size() +
			specification.attractionPointCount()));
	std::unordered_set<std::string> node_identifiers;
	for (const BranchNode &source_node : request.sourceGraph().nodes()) {
		nodes.push_back(MutableBranchNode{
			source_node.identifier(), source_node.position(), source_node.radius(),
			source_node.developmentalAge(), source_node.branchOrder(),
			source_node.state()});
		node_identifiers.insert(source_node.identifier());
	}
	std::vector<BranchSegment> segments = request.sourceGraph().segments();
	std::unordered_set<std::string> segment_identifiers;
	std::unordered_map<std::string, std::size_t> outgoing_count;
	for (const BranchSegment &segment : segments) {
		segment_identifiers.insert(segment.identifier());
		++outgoing_count[segment.parentNodeIdentifier()];
	}

	std::unordered_set<std::string> extended_node_identifiers;
	std::unordered_map<std::string, glm::vec3> generated_tip_directions;
	std::size_t node_identifier_counter = 0u;
	std::size_t segment_identifier_counter = 0u;
	std::size_t obstacle_rejected_candidate_count = 0u;
	std::size_t completed_iterations = 0u;
	const std::size_t source_node_count = nodes.size();

	for (std::size_t iteration = 0;
	     iteration < specification.maximumIterations() &&
	     !attraction_points.empty();
	     ++iteration) {
		completed_iterations = iteration + 1u;
		const NodeSpatialGrid grid = build_spatial_grid(
			nodes, specification.influenceRadius());
		std::vector<AttractionAssignment> assignments(nodes.size());
		std::vector<glm::vec3> surviving_attraction_points;
		surviving_attraction_points.reserve(attraction_points.size());

		for (const glm::vec3 &attraction_point : attraction_points) {
			std::size_t nearest_index = nodes.size();
			float nearest_distance = std::numeric_limits<float>::max();
			for (const std::size_t node_index : neighboring_nodes(
				     grid, attraction_point, specification.influenceRadius())) {
				if (!node_can_grow(nodes[node_index])) continue;
				const float distance = glm::distance(
					nodes[node_index].position, attraction_point);
				if (distance < nearest_distance - 1.0e-7f ||
				    (std::fabs(distance - nearest_distance) <= 1.0e-7f &&
				     (nearest_index == nodes.size() ||
				      nodes[node_index].identifier <
					      nodes[nearest_index].identifier))) {
					nearest_index = node_index;
					nearest_distance = distance;
				}
			}
			if (nearest_index != nodes.size() &&
			    nearest_distance <= specification.killRadius()) {
				continue;
			}
			surviving_attraction_points.push_back(attraction_point);
			if (nearest_index == nodes.size() ||
			    nearest_distance > specification.influenceRadius()) {
				continue;
			}
			assignments[nearest_index].direction_sum += glm::normalize(
				attraction_point - nodes[nearest_index].position);
			++assignments[nearest_index].count;
		}
		attraction_points = std::move(surviving_attraction_points);

		std::vector<MutableBranchNode> new_nodes;
		std::vector<BranchSegment> new_segments;
		for (std::size_t parent_index = 0;
		     parent_index < assignments.size();
		     ++parent_index) {
			if (assignments[parent_index].count == 0u) continue;
			glm::vec3 desired_direction =
				assignments[parent_index].direction_sum /
				static_cast<float>(assignments[parent_index].count);
			if (specification.tropismWeight() > 0.0f) {
				desired_direction += specification.tropismWeight() *
					glm::normalize(specification.tropismDirection());
			}
			if (glm::length(desired_direction) <= 1.0e-6f) continue;
			desired_direction = glm::normalize(desired_direction);

			glm::vec3 resolved_position(0.0f);
			glm::vec3 resolved_direction(0.0f);
			if (!candidate_position(
					nodes[parent_index], desired_direction,
					specification.stepLength(), request.crownVolume(),
					request.obstacles(), nodes, new_nodes,
					&obstacle_rejected_candidate_count, &resolved_position,
					&resolved_direction)) {
				continue;
			}
			if (nodes.size() + new_nodes.size() >=
				    complexity_limits_.maximumBranchNodes() ||
			    segments.size() + new_segments.size() >=
				    complexity_limits_.maximumBranchSegments()) {
				return SpaceColonizationResult::failed(
					"Space colonization exceeded the branch graph complexity ceiling.");
			}

			const std::string node_identifier = next_unique_identifier(
				"colonized_node_", &node_identifier_counter,
				&node_identifiers);
			const std::string segment_identifier = next_unique_identifier(
				"colonized_segment_", &segment_identifier_counter,
				&segment_identifiers);
			const bool continuation =
				outgoing_count[nodes[parent_index].identifier] == 0u;
			const int branch_order = nodes[parent_index].branch_order +
				(continuation ? 0 : 1);
			new_nodes.push_back(MutableBranchNode{
				node_identifier, resolved_position,
				std::max(
					nodes[parent_index].radius * specification.radiusDecay(),
					specification.minimumRadius()),
				nodes[parent_index].developmental_age + 1.0f,
				branch_order, BranchNodeState::Active});
			new_segments.emplace_back(
				segment_identifier, nodes[parent_index].identifier,
				node_identifier,
				continuation
					? BranchSegmentKind::Continuation
					: BranchSegmentKind::LateralBranch);
			++outgoing_count[nodes[parent_index].identifier];
			extended_node_identifiers.insert(nodes[parent_index].identifier);
			generated_tip_directions[node_identifier] = resolved_direction;
		}

		if (new_nodes.empty()) break;
		for (const MutableBranchNode &new_node : new_nodes) {
			nodes.push_back(new_node);
		}
		for (const BranchSegment &new_segment : new_segments) {
			segments.push_back(new_segment);
		}
	}

	reconcile_radii(
		&nodes, segments, request.sourceGraph().rootNodeIdentifier(),
		specification.minimumRadius(),
		specification.radiusConservationExponent());

	std::vector<VegetationGrowthTip> growth_tips;
	growth_tips.reserve(request.sourceGraph().growthTips().size() +
	                    nodes.size() - source_node_count);
	for (const VegetationGrowthTip &source_tip :
	     request.sourceGraph().growthTips()) {
		growth_tips.emplace_back(
			source_tip.identifier(), source_tip.hostNodeIdentifier(),
			source_tip.direction(),
			source_tip.isActive() &&
				extended_node_identifiers.find(source_tip.hostNodeIdentifier()) ==
					extended_node_identifiers.end());
	}
	for (std::size_t node_index = source_node_count;
	     node_index < nodes.size();
	     ++node_index) {
		const MutableBranchNode &node = nodes[node_index];
		if (outgoing_count[node.identifier] != 0u) continue;
		growth_tips.emplace_back(
			"tip_" + node.identifier, node.identifier,
			generated_tip_directions.at(node.identifier), true);
	}
	if (growth_tips.size() > complexity_limits_.maximumGrowthTips()) {
		return SpaceColonizationResult::failed(
			"Space colonization exceeded the growth-tip complexity ceiling.");
	}

	BranchGraph resolved_graph(
		request.sourceGraph().rootNodeIdentifier(), immutable_nodes(nodes),
		std::move(segments), request.sourceGraph().buds(),
		request.sourceGraph().organAttachments(), std::move(growth_tips));
	const BranchGraphValidationReport resolved_validation =
		validation_service.validate(resolved_graph);
	if (!resolved_validation.succeeded()) {
		return SpaceColonizationResult::failed(
			"Space colonization produced an invalid graph: " +
			resolved_validation.issues().front().diagnostic());
	}
	const BranchGraphValidationReport radius_validation =
		BranchRadiusConservationService().validate(
			resolved_graph, specification.radiusConservationExponent(), 0.0001f);
	if (!radius_validation.succeeded()) {
		return SpaceColonizationResult::failed(
			"Space colonization could not reconcile branch radii: " +
			radius_validation.issues().front().diagnostic());
	}
	for (std::size_t node_index = source_node_count;
	     node_index < resolved_graph.nodes().size();
	     ++node_index) {
		if (!containment_service.contains(
				request.crownVolume(), resolved_graph.nodes()[node_index].position())) {
			return SpaceColonizationResult::failed(
				"Space colonization produced a node outside the crown volume.");
		}
	}

	const BranchGraphDeterministicHashService hash_service;
	const std::uint64_t source_graph_hash =
		hash_service.hash(request.sourceGraph());
	const std::uint64_t resolved_graph_hash = hash_service.hash(resolved_graph);
	const std::size_t added_node_count =
		resolved_graph.nodes().size() - source_node_count;
	const std::uint64_t evidence_hash = resolution_evidence_hash(
		source_graph_hash, resolved_graph_hash, request.deterministicSeed(),
		specification.attractionPointCount(), attraction_points.size(),
		completed_iterations, added_node_count,
		obstacle_rejected_candidate_count);
	return SpaceColonizationResult::succeeded(SpaceColonizationSnapshot(
		std::move(resolved_graph),
		SpaceColonizationResolutionEvidence(
			source_graph_hash, resolved_graph_hash, evidence_hash,
			specification.attractionPointCount(), attraction_points.size(),
			completed_iterations, added_node_count,
			obstacle_rejected_candidate_count)));
}
