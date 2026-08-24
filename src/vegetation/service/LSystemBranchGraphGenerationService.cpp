#include "vegetation/service/LSystemBranchGraphGenerationService.h"

#include "vegetation/model/BranchNodeState.h"
#include "vegetation/model/BranchSegmentKind.h"
#include "vegetation/model/VegetationBud.h"
#include "vegetation/model/VegetationGrowthTip.h"
#include "vegetation/model/VegetationOrganType.h"
#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/BranchRadiusConservationService.h"
#include "vegetation/service/PlantLSystemSpecificationValidationService.h"
#include "vegetation/service/TropismDirectionService.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct MutableLSystemBranchNode
{
	std::string identifier;
	glm::vec3 position{0.0f};
	glm::vec3 growth_direction{0.0f, 1.0f, 0.0f};
	float radius = 0.0f;
	float developmental_age = 0.0f;
	int branch_order = 0;
	std::size_t path_depth = 0u;
	BranchNodeState state = BranchNodeState::Dormant;
};

struct LSystemTurtleState
{
	std::size_t node_index = 0u;
	glm::vec3 position{0.0f};
	glm::vec3 direction{0.0f, 1.0f, 0.0f};
	int branch_order = 0;
	std::size_t path_depth = 0u;
	BranchSegmentKind next_segment_kind = BranchSegmentKind::Continuation;
};

std::size_t symbol_index(LSystemSymbol symbol)
{
	return static_cast<std::size_t>(symbol);
}

BranchNodeState node_state(PlantDevelopmentState development_state)
{
	switch (development_state) {
	case PlantDevelopmentState::Seed:
	case PlantDevelopmentState::Bud:
	case PlantDevelopmentState::Dormant:
		return BranchNodeState::Dormant;
	case PlantDevelopmentState::Shoot:
	case PlantDevelopmentState::Juvenile:
		return BranchNodeState::Active;
	case PlantDevelopmentState::Mature:
	case PlantDevelopmentState::Flowering:
	case PlantDevelopmentState::Fruiting:
		return BranchNodeState::Mature;
	case PlantDevelopmentState::Senescent:
		return BranchNodeState::Senescent;
	case PlantDevelopmentState::Dead:
		return BranchNodeState::Dead;
	}
	return BranchNodeState::Dormant;
}

bool growth_tip_is_active(PlantDevelopmentState development_state)
{
	return development_state != PlantDevelopmentState::Seed &&
	       development_state != PlantDevelopmentState::Bud &&
	       development_state != PlantDevelopmentState::Dormant &&
	       development_state != PlantDevelopmentState::Senescent &&
	       development_state != PlantDevelopmentState::Dead;
}

glm::vec3 rotate_around_z(glm::vec3 direction, float angle_degrees)
{
	const float angle = glm::radians(angle_degrees);
	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	return glm::normalize(glm::vec3(
		cosine * direction.x - sine * direction.y,
		sine * direction.x + cosine * direction.y,
		direction.z));
}

std::vector<LSystemSymbol> expand_sentence(
	const PlantLSystemSpecification &specification)
{
	std::array<const std::vector<LSystemSymbol> *, 5> successors{};
	for (const LSystemProductionRule &rule : specification.productionRules()) {
		successors[symbol_index(rule.predecessor())] = &rule.successor();
	}
	std::vector<LSystemSymbol> sentence = specification.axiom();
	for (std::size_t iteration = 0u;
	     iteration < specification.iterationCount();
	     ++iteration) {
		std::vector<LSystemSymbol> next;
		for (LSystemSymbol symbol : sentence) {
			const auto *successor = successors[symbol_index(symbol)];
			if (successor == nullptr) next.push_back(symbol);
			else next.insert(next.end(), successor->begin(), successor->end());
		}
		sentence = std::move(next);
	}
	return sentence;
}

std::vector<BranchNode> immutable_nodes(
	const std::vector<MutableLSystemBranchNode> &nodes)
{
	std::vector<BranchNode> result;
	result.reserve(nodes.size());
	for (const MutableLSystemBranchNode &node : nodes) {
		result.emplace_back(
			node.identifier, node.position, node.radius,
			node.developmental_age, node.branch_order, node.state);
	}
	return result;
}

void reconcile_branch_radii(
	std::vector<MutableLSystemBranchNode> *nodes,
	const std::vector<BranchSegment> &segments,
	const PlantLSystemSpecification &specification)
{
	std::size_t maximum_depth = 0u;
	for (const MutableLSystemBranchNode &node : *nodes) {
		maximum_depth = std::max(maximum_depth, node.path_depth);
	}
	for (MutableLSystemBranchNode &node : *nodes) {
		const float progress = maximum_depth == 0u
			? 0.0f
			: static_cast<float>(node.path_depth) /
				static_cast<float>(maximum_depth);
		node.radius = specification.terminalRadius() +
		              (specification.baseRadius() - specification.terminalRadius()) *
			              (1.0f - progress);
	}

	std::unordered_map<std::string, std::size_t> node_indices;
	for (std::size_t index = 0u; index < nodes->size(); ++index) {
		node_indices.emplace((*nodes)[index].identifier, index);
	}
	std::unordered_map<std::string, std::vector<std::string>> children;
	for (const BranchSegment &segment : segments) {
		children[segment.parentNodeIdentifier()].push_back(
			segment.childNodeIdentifier());
	}
	const BranchGraph preliminary(
		nodes->front().identifier, immutable_nodes(*nodes), segments);
	const std::vector<std::string> traversal =
		BranchGraphValidationService().deterministicTraversal(preliminary);
	for (auto iterator = traversal.rbegin(); iterator != traversal.rend(); ++iterator) {
		MutableLSystemBranchNode &node = (*nodes)[node_indices.at(*iterator)];
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
				specification.radiusConservationExponent());
		}
		node.radius = std::pow(
			child_measure,
			1.0f / specification.radiusConservationExponent());
	}
	const float root_radius = nodes->front().radius;
	const float scale = root_radius > 0.0f
		? specification.baseRadius() / root_radius : 1.0f;
	for (MutableLSystemBranchNode &node : *nodes) node.radius *= scale;
}

}

LSystemBranchGraphGenerationResult LSystemBranchGraphGenerationService::generate(
	const PlantLSystemSpecification &specification,
	float developmental_age,
	PlantDevelopmentState development_state,
	const std::vector<TropismInfluence> &tropism_influences) const
{
	std::string diagnostic;
	if (!std::isfinite(developmental_age) || developmental_age < 0.0f) {
		return LSystemBranchGraphGenerationResult::failed(
			"Plant L-system generation requires non-negative finite developmental age.");
	}
	if (!PlantLSystemSpecificationValidationService(complexity_limits_).validate(
			specification, &diagnostic)) {
		return LSystemBranchGraphGenerationResult::failed(diagnostic);
	}

	const std::vector<LSystemSymbol> sentence = expand_sentence(specification);
	std::vector<MutableLSystemBranchNode> nodes;
	std::vector<BranchSegment> segments;
	std::vector<LSystemTurtleState> stack;
	std::size_t maximum_stack_depth = 0u;
	const BranchNodeState resolved_state = node_state(development_state);
	nodes.push_back(MutableLSystemBranchNode{
		"lsystem_node_0", glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		specification.baseRadius(), 0.0f, 0, 0u, resolved_state});
	LSystemTurtleState turtle;

	for (LSystemSymbol symbol : sentence) {
		switch (symbol) {
		case LSystemSymbol::Forward: {
			const TropismResolution tropism = TropismDirectionService().resolve(
				turtle.direction, tropism_influences);
			if (!tropism.succeeded()) {
				return LSystemBranchGraphGenerationResult::failed(
					"Plant L-system tropism failed: " + tropism.diagnostic());
			}
			turtle.direction = tropism.direction();
			const glm::vec3 next_position =
				turtle.position + turtle.direction * specification.stepLength();
			const std::size_t child_index = nodes.size();
			const std::size_t child_depth = turtle.path_depth + 1u;
			const float node_age = developmental_age *
				static_cast<float>(child_index) /
				static_cast<float>(std::max<std::size_t>(1u, sentence.size()));
			nodes.push_back(MutableLSystemBranchNode{
				"lsystem_node_" + std::to_string(child_index), next_position,
				turtle.direction, specification.terminalRadius(), node_age,
				turtle.branch_order, child_depth, resolved_state});
			segments.emplace_back(
				"lsystem_segment_" + std::to_string(segments.size()),
				nodes[turtle.node_index].identifier, nodes[child_index].identifier,
				turtle.next_segment_kind);
			turtle.node_index = child_index;
			turtle.position = next_position;
			turtle.path_depth = child_depth;
			turtle.next_segment_kind = BranchSegmentKind::Continuation;
			break;
		}
		case LSystemSymbol::TurnPositive:
			turtle.direction = rotate_around_z(
				turtle.direction, specification.turnAngleDegrees());
			break;
		case LSystemSymbol::TurnNegative:
			turtle.direction = rotate_around_z(
				turtle.direction, -specification.turnAngleDegrees());
			break;
		case LSystemSymbol::PushState:
			stack.push_back(turtle);
			maximum_stack_depth = std::max(maximum_stack_depth, stack.size());
			++turtle.branch_order;
			turtle.next_segment_kind = BranchSegmentKind::LateralBranch;
			break;
		case LSystemSymbol::PopState:
			if (stack.empty()) {
				return LSystemBranchGraphGenerationResult::failed(
					"Plant L-system generation encountered an empty turtle-state stack.");
			}
			turtle = stack.back();
			stack.pop_back();
			break;
		}
	}
	if (!stack.empty()) {
		return LSystemBranchGraphGenerationResult::failed(
			"Plant L-system generation ended with unmatched turtle-state pushes.");
	}

	reconcile_branch_radii(&nodes, segments, specification);
	std::vector<std::size_t> outgoing_counts(nodes.size(), 0u);
	std::unordered_map<std::string, std::size_t> node_indices;
	for (std::size_t index = 0u; index < nodes.size(); ++index) {
		node_indices.emplace(nodes[index].identifier, index);
	}
	for (const BranchSegment &segment : segments) {
		++outgoing_counts[node_indices.at(segment.parentNodeIdentifier())];
	}
	std::vector<VegetationBud> buds;
	std::vector<VegetationGrowthTip> growth_tips;
	for (std::size_t index = 0u; index < nodes.size(); ++index) {
		if (outgoing_counts[index] != 0u) continue;
		buds.emplace_back(
			"lsystem_bud_" + std::to_string(buds.size()), nodes[index].identifier,
			nodes[index].growth_direction, developmental_age,
			VegetationBudState::BranchBud, VegetationOrganType::Bud, 1.0f);
		growth_tips.emplace_back(
			"lsystem_tip_" + std::to_string(growth_tips.size()),
			nodes[index].identifier, nodes[index].growth_direction,
			growth_tip_is_active(development_state));
	}

	BranchGraph graph(
		nodes.front().identifier, immutable_nodes(nodes), std::move(segments),
		std::move(buds), {}, std::move(growth_tips));
	const BranchGraphValidationReport validation =
		BranchGraphValidationService(complexity_limits_).validate(graph);
	if (!validation.succeeded()) {
		return LSystemBranchGraphGenerationResult::failed(
			"Plant L-system generated an invalid BranchGraph: " +
			validation.issues().front().diagnostic());
	}
	const BranchGraphValidationReport radius_validation =
		BranchRadiusConservationService().validate(
			graph, specification.radiusConservationExponent(), 0.0001f);
	if (!radius_validation.succeeded()) {
		return LSystemBranchGraphGenerationResult::failed(
			"Plant L-system could not reconcile branch radii: " +
			radius_validation.issues().front().diagnostic());
	}
	const std::uint64_t topology_hash =
		BranchGraphDeterministicHashService().hash(graph);
	return LSystemBranchGraphGenerationResult::succeeded(
		LSystemBranchGraphGenerationSnapshot(
			std::move(graph),
			LSystemBranchGraphGenerationEvidence(
				specification.iterationCount(), sentence.size(), maximum_stack_depth,
				nodes.size() - 1u, topology_hash)));
}
