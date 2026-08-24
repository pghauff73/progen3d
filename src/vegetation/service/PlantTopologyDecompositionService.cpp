#include "vegetation/service/PlantTopologyDecompositionService.h"

#include "vegetation/model/BranchGraph.h"
#include "vegetation/service/BranchGraphValidationService.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

PlantTopologyDecomposition PlantTopologyDecompositionService::decompose(
	const BranchGraph &graph) const
{
	const BranchGraphValidationReport validation =
		BranchGraphValidationService().validate(graph);
	if (!validation.succeeded()) {
		return PlantTopologyDecomposition::failed(
			"Plant topology decomposition requires a valid BranchGraph: " +
			validation.issues().front().diagnostic());
	}

	std::unordered_map<std::string, const BranchNode *> nodes;
	for (const BranchNode &node : graph.nodes()) {
		nodes.emplace(node.identifier(), &node);
	}
	std::unordered_map<std::string, const BranchSegment *> incoming;
	std::unordered_map<std::string, std::vector<const BranchSegment *>> outgoing;
	for (const BranchSegment &segment : graph.segments()) {
		incoming.emplace(segment.childNodeIdentifier(), &segment);
		outgoing[segment.parentNodeIdentifier()].push_back(&segment);
	}

	std::vector<PlantInternode> internodes;
	internodes.reserve(graph.segments().size());
	std::unordered_map<std::string, std::size_t> internode_index_by_segment;
	for (const BranchSegment &segment : graph.segments()) {
		const BranchNode *start = nodes.at(segment.parentNodeIdentifier());
		const BranchNode *end = nodes.at(segment.childNodeIdentifier());
		internode_index_by_segment.emplace(segment.identifier(), internodes.size());
		internodes.emplace_back(
			"internode_" + segment.identifier(), segment.identifier(),
			start->identifier(), end->identifier(), start->position(), end->position(),
			start->radius(), end->radius(), end->branchOrder(), segment.kind());
	}

	std::vector<const BranchSegment *> stem_starts;
	for (const BranchSegment &segment : graph.segments()) {
		const auto parent_incoming = incoming.find(segment.parentNodeIdentifier());
		if (segment.parentNodeIdentifier() == graph.rootNodeIdentifier() ||
		    segment.kind() == BranchSegmentKind::LateralBranch ||
		    parent_incoming == incoming.end() ||
		    parent_incoming->second->kind() == BranchSegmentKind::LateralBranch) {
			stem_starts.push_back(&segment);
		}
	}

	std::vector<PlantStem> stems;
	std::unordered_set<std::string> assigned_segments;
	for (const BranchSegment *start_segment : stem_starts) {
		if (!assigned_segments.insert(start_segment->identifier()).second) continue;
		std::vector<PlantInternode> stem_internodes;
		const BranchSegment *current = start_segment;
		const BranchNode *first_end = nodes.at(current->childNodeIdentifier());
		const int branch_order = first_end->branchOrder();
		while (current != nullptr) {
			stem_internodes.push_back(
				internodes[internode_index_by_segment.at(current->identifier())]);
			const auto found = outgoing.find(current->childNodeIdentifier());
			const BranchSegment *continuation = nullptr;
			if (found != outgoing.end()) {
				for (const BranchSegment *candidate : found->second) {
					const BranchNode *candidate_end =
						nodes.at(candidate->childNodeIdentifier());
					if (candidate->kind() == BranchSegmentKind::Continuation &&
					    candidate_end->branchOrder() == branch_order &&
					    assigned_segments.find(candidate->identifier()) ==
						    assigned_segments.end()) {
						continuation = candidate;
						break;
					}
				}
			}
			if (continuation != nullptr) {
				assigned_segments.insert(continuation->identifier());
			}
			current = continuation;
		}
		stems.emplace_back(
			"stem_" + start_segment->identifier(), branch_order,
			std::move(stem_internodes));
	}

	for (const BranchSegment &segment : graph.segments()) {
		if (assigned_segments.find(segment.identifier()) != assigned_segments.end()) {
			continue;
		}
		const PlantInternode &internode =
			internodes[internode_index_by_segment.at(segment.identifier())];
		stems.emplace_back(
			"stem_" + segment.identifier(), internode.branchOrder(),
			std::vector<PlantInternode>{internode});
	}

	return PlantTopologyDecomposition::succeeded(
		std::move(internodes), std::move(stems));
}
