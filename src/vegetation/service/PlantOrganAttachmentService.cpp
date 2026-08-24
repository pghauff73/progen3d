#include "vegetation/service/PlantOrganAttachmentService.h"

#include "vegetation/service/OrganPlacementService.h"

#include <glm/geometric.hpp>

#include <unordered_map>
#include <utility>
#include <vector>

std::optional<BranchGraph> PlantOrganAttachmentService::attachLeaves(
	const BranchGraph &graph,
	const PlantSpeciesSpecification &species,
	float developmental_age,
	std::string *diagnostic) const
{
	std::unordered_map<std::string, const BranchNode *> nodes_by_identifier;
	for (const BranchNode &node : graph.nodes()) {
		nodes_by_identifier.emplace(node.identifier(), &node);
	}
	std::unordered_map<std::string, std::string> parent_by_child;
	for (const BranchSegment &segment : graph.segments()) {
		parent_by_child.emplace(
			segment.childNodeIdentifier(), segment.parentNodeIdentifier());
	}

	std::vector<OrganAttachment> attachments = graph.organAttachments();
	std::size_t phyllotaxis_node_index = 0u;
	for (const BranchNode &node : graph.nodes()) {
		if (node.identifier() == graph.rootNodeIdentifier() ||
		    node.branchOrder() < species.leaf().minimumBranchOrder()) {
			continue;
		}
		const auto parent_identifier = parent_by_child.find(node.identifier());
		if (parent_identifier == parent_by_child.end()) continue;
		const auto parent = nodes_by_identifier.find(parent_identifier->second);
		if (parent == nodes_by_identifier.end()) continue;
		const glm::vec3 axis = node.position() - parent->second->position();
		if (glm::length(axis) <= 1.0e-6f) continue;

		std::string placement_diagnostic;
		const std::vector<VegetationOrganPlacement> placements =
			OrganPlacementService().placeAtNode(
				species.identifier() + ":leaf:" + node.identifier(),
				phyllotaxis_node_index, node.position(), axis,
				species.leaf().phyllotaxis(), &placement_diagnostic);
		if (!placement_diagnostic.empty()) {
			if (diagnostic != nullptr) *diagnostic = placement_diagnostic;
			return std::nullopt;
		}
		if (attachments.size() + placements.size() >
		    complexity_limits_.maximumOrganAttachments()) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Plant leaf attachment exceeded the organ-attachment safety limit.";
			}
			return std::nullopt;
		}
		for (const VegetationOrganPlacement &placement : placements) {
			attachments.emplace_back(
				placement.identifier(), node.identifier(), VegetationOrganType::Leaf,
				placement.localTransform(), developmental_age,
				species.identifier() + ":leaf", "node_surface", "petiole_base");
		}
		++phyllotaxis_node_index;
	}

	if (diagnostic != nullptr) diagnostic->clear();
	return BranchGraph(
		graph.rootNodeIdentifier(), graph.nodes(), graph.segments(), graph.buds(),
		std::move(attachments), graph.growthTips());
}
