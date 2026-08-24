#include "vegetation/service/BranchGraphValidationService.h"

#include "vegetation/model/BranchGraph.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite(const glm::mat4 &value)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(value[column][row])) return false;
		}
	}
	return true;
}

template <typename Record>
bool register_identifier(
	const Record &record,
	const char *record_kind,
	std::unordered_set<std::string> *identifiers,
	BranchGraphValidationReport *report)
{
	if (record.identifier().empty()) {
		report->addIssue(
			BranchGraphDiagnosticCode::EmptyIdentifier,
			std::string(record_kind) + " requires a non-empty identifier.");
		return false;
	}
	if (!identifiers->insert(record.identifier()).second) {
		report->addIssue(
			BranchGraphDiagnosticCode::DuplicateIdentifier,
			std::string(record_kind) + " identifier '" + record.identifier() +
			"' is duplicated.");
		return false;
	}
	return true;
}

} // namespace

BranchGraphValidationReport BranchGraphValidationService::validate(
	const BranchGraph &graph) const
{
	BranchGraphValidationReport report;
	if (graph.nodes().size() > complexity_limits_.maximumBranchNodes() ||
	    graph.segments().size() > complexity_limits_.maximumBranchSegments() ||
	    graph.buds().size() > complexity_limits_.maximumBuds() ||
	    graph.organAttachments().size() >
		    complexity_limits_.maximumOrganAttachments() ||
	    graph.growthTips().size() > complexity_limits_.maximumGrowthTips()) {
		report.addIssue(
			BranchGraphDiagnosticCode::ComplexityLimitExceeded,
			"BranchGraph exceeds the configured vegetation complexity limits.");
		return report;
	}

	std::unordered_map<std::string, const BranchNode *> nodes;
	std::unordered_set<std::string> node_identifiers;
	for (const BranchNode &node : graph.nodes()) {
		register_identifier(node, "BranchNode", &node_identifiers, &report);
		if (!node.identifier().empty()) nodes.emplace(node.identifier(), &node);
		if (!finite(node.position()) || !std::isfinite(node.radius()) ||
		    !std::isfinite(node.developmentalAge())) {
			report.addIssue(
				BranchGraphDiagnosticCode::NonFiniteValue,
				"BranchNode '" + node.identifier() +
				"' contains a non-finite value.");
		}
		if (node.radius() <= 0.0f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidRadius,
				"BranchNode '" + node.identifier() +
				"' requires a positive radius.");
		}
		if (node.developmentalAge() < 0.0f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidDevelopmentalAge,
				"BranchNode '" + node.identifier() +
				"' requires a non-negative developmental age.");
		}
		if (node.branchOrder() < 0) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidBranchOrder,
				"BranchNode '" + node.identifier() +
				"' requires a non-negative branch order.");
		}
	}

	const auto root = nodes.find(graph.rootNodeIdentifier());
	if (graph.rootNodeIdentifier().empty() || root == nodes.end()) {
		report.addIssue(
			BranchGraphDiagnosticCode::MissingRoot,
			"BranchGraph root must reference an existing BranchNode.");
	}
	else if (root->second->branchOrder() != 0) {
		report.addIssue(
			BranchGraphDiagnosticCode::InvalidBranchOrder,
			"BranchGraph root must have branch order zero.");
	}

	std::unordered_set<std::string> segment_identifiers;
	std::unordered_map<std::string, std::vector<const BranchSegment *>> outgoing;
	std::unordered_map<std::string, int> incoming_count;
	for (const BranchSegment &segment : graph.segments()) {
		register_identifier(segment, "BranchSegment", &segment_identifiers, &report);
		const auto parent = nodes.find(segment.parentNodeIdentifier());
		const auto child = nodes.find(segment.childNodeIdentifier());
		if (parent == nodes.end() || child == nodes.end()) {
			report.addIssue(
				BranchGraphDiagnosticCode::MissingNodeReference,
				"BranchSegment '" + segment.identifier() +
				"' references a missing node.");
			continue;
		}
		if (parent == child) {
			report.addIssue(
				BranchGraphDiagnosticCode::CycleDetected,
				"BranchSegment '" + segment.identifier() +
				"' cannot connect a node to itself.");
			continue;
		}
		outgoing[parent->first].push_back(&segment);
		if (++incoming_count[child->first] > 1) {
			report.addIssue(
				BranchGraphDiagnosticCode::MultipleParents,
				"BranchNode '" + child->first +
				"' has more than one parent segment.");
		}
		const int expected_order =
			segment.kind() == BranchSegmentKind::Continuation
				? parent->second->branchOrder()
				: parent->second->branchOrder() + 1;
		if (child->second->branchOrder() != expected_order) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidSegmentOrder,
				"BranchSegment '" + segment.identifier() +
				"' has inconsistent parent/child branch orders.");
		}
		if (child->second->developmentalAge() <
		    parent->second->developmentalAge()) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidDevelopmentalAge,
				"BranchSegment '" + segment.identifier() +
				"' runs from a newer node to an older node.");
		}
		if (glm::length(child->second->position() - parent->second->position()) <=
		    1.0e-6f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidAttachment,
				"BranchSegment '" + segment.identifier() +
				"' requires spatially distinct nodes.");
		}
	}

	if (root != nodes.end() && incoming_count[root->first] != 0) {
		report.addIssue(
			BranchGraphDiagnosticCode::InvalidRootParent,
			"BranchGraph root cannot have a parent segment.");
	}
	for (const auto &entry : nodes) {
		if (entry.first != graph.rootNodeIdentifier() &&
		    incoming_count[entry.first] != 1) {
			report.addIssue(
				BranchGraphDiagnosticCode::DisconnectedNode,
				"BranchNode '" + entry.first +
				"' must have exactly one parent segment.");
		}
	}

	std::unordered_set<std::string> attachment_identifiers;
	for (const OrganAttachment &attachment : graph.organAttachments()) {
		register_identifier(
			attachment, "OrganAttachment", &attachment_identifiers, &report);
		if (nodes.find(attachment.hostNodeIdentifier()) == nodes.end() ||
		    attachment.shapeIdentifier().empty() ||
		    attachment.hostInterfaceIdentifier().empty() ||
		    attachment.organInterfaceIdentifier().empty() ||
		    !finite(attachment.localTransform()) ||
		    !std::isfinite(attachment.developmentalAge()) ||
		    attachment.developmentalAge() < 0.0f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidAttachment,
				"OrganAttachment '" + attachment.identifier() +
				" has an invalid host, interface, shape, transform, or age.");
		}
	}

	std::unordered_set<std::string> bud_identifiers;
	for (const VegetationBud &bud : graph.buds()) {
		register_identifier(bud, "VegetationBud", &bud_identifiers, &report);
		if (nodes.find(bud.hostNodeIdentifier()) == nodes.end() ||
		    !finite(bud.direction()) || glm::length(bud.direction()) <= 1.0e-6f ||
		    !std::isfinite(bud.developmentalAge()) ||
		    !std::isfinite(bud.activationProbability()) ||
		    bud.developmentalAge() < 0.0f || bud.activationProbability() < 0.0f ||
		    bud.activationProbability() > 1.0f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidBud,
				"VegetationBud '" + bud.identifier() +
				" has invalid host, direction, age, or probability.");
		}
	}

	std::unordered_set<std::string> growth_tip_identifiers;
	for (const VegetationGrowthTip &growth_tip : graph.growthTips()) {
		register_identifier(
			growth_tip, "VegetationGrowthTip", &growth_tip_identifiers, &report);
		if (nodes.find(growth_tip.hostNodeIdentifier()) == nodes.end() ||
		    !finite(growth_tip.direction()) ||
		    glm::length(growth_tip.direction()) <= 1.0e-6f) {
			report.addIssue(
				BranchGraphDiagnosticCode::InvalidGrowthTip,
				"VegetationGrowthTip '" + growth_tip.identifier() +
				" has an invalid host or direction.");
		}
	}

	std::string traversal_diagnostic;
	const std::vector<std::string> traversal =
		deterministicTraversal(graph, &traversal_diagnostic);
	if (traversal.empty() && !graph.nodes().empty()) {
		report.addIssue(
			BranchGraphDiagnosticCode::CycleDetected,
			traversal_diagnostic.empty()
				? "BranchGraph traversal failed."
				: traversal_diagnostic);
	}
	else if (traversal.size() != nodes.size()) {
		report.addIssue(
			BranchGraphDiagnosticCode::DisconnectedNode,
			"BranchGraph contains nodes that are unreachable from the root.");
	}
	return report;
}

std::vector<std::string> BranchGraphValidationService::deterministicTraversal(
	const BranchGraph &graph,
	std::string *diagnostic) const
{
	std::unordered_map<std::string, const BranchNode *> nodes;
	for (const BranchNode &node : graph.nodes()) {
		if (!node.identifier().empty() &&
		    nodes.emplace(node.identifier(), &node).second == false) {
			if (diagnostic != nullptr) {
				*diagnostic = "BranchGraph traversal requires unique node identifiers.";
			}
			return {};
		}
	}
	if (nodes.find(graph.rootNodeIdentifier()) == nodes.end()) {
		if (diagnostic != nullptr) {
			*diagnostic = "BranchGraph traversal requires an existing root node.";
		}
		return {};
	}

	std::unordered_map<std::string, std::vector<std::string>> children;
	for (const BranchSegment &segment : graph.segments()) {
		if (nodes.find(segment.parentNodeIdentifier()) == nodes.end() ||
		    nodes.find(segment.childNodeIdentifier()) == nodes.end()) {
			if (diagnostic != nullptr) {
				*diagnostic = "BranchGraph traversal encountered a missing node reference.";
			}
			return {};
		}
		children[segment.parentNodeIdentifier()].push_back(
			segment.childNodeIdentifier());
	}
	for (auto &entry : children) {
		std::sort(entry.second.begin(), entry.second.end());
	}

	std::unordered_set<std::string> visiting;
	std::unordered_set<std::string> visited;
	std::vector<std::string> order;
	std::function<bool(const std::string &)> visit =
		[&](const std::string &identifier) {
			if (visiting.find(identifier) != visiting.end()) return false;
			if (visited.find(identifier) != visited.end()) return true;
			visiting.insert(identifier);
			visited.insert(identifier);
			order.push_back(identifier);
			for (const std::string &child : children[identifier]) {
				if (!visit(child)) return false;
			}
			visiting.erase(identifier);
			return true;
		};
	if (!visit(graph.rootNodeIdentifier())) {
		if (diagnostic != nullptr) {
			*diagnostic = "BranchGraph contains a parent-child cycle.";
		}
		return {};
	}
	return order;
}
