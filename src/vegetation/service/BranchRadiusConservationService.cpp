#include "vegetation/service/BranchRadiusConservationService.h"

#include "vegetation/model/BranchGraph.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

BranchGraphValidationReport BranchRadiusConservationService::validate(
	const BranchGraph &graph,
	float exponent_gamma,
	float relative_tolerance) const
{
	BranchGraphValidationReport report;
	if (!std::isfinite(exponent_gamma) || exponent_gamma <= 0.0f ||
	    !std::isfinite(relative_tolerance) || relative_tolerance < 0.0f) {
		report.addIssue(
			BranchGraphDiagnosticCode::RadiusConservationViolation,
			"Branch radius conservation requires positive finite gamma and tolerance.");
		return report;
	}

	std::unordered_map<std::string, const BranchNode *> nodes;
	for (const BranchNode &node : graph.nodes()) nodes[node.identifier()] = &node;
	std::unordered_map<std::string, std::vector<const BranchNode *>> children;
	for (const BranchSegment &segment : graph.segments()) {
		const auto parent = nodes.find(segment.parentNodeIdentifier());
		const auto child = nodes.find(segment.childNodeIdentifier());
		if (parent != nodes.end() && child != nodes.end()) {
			children[parent->first].push_back(child->second);
		}
	}

	for (const auto &entry : children) {
		if (entry.second.size() < 2u) continue;
		const BranchNode *parent = nodes[entry.first];
		const float parent_measure = std::pow(parent->radius(), exponent_gamma);
		float child_measure = 0.0f;
		for (const BranchNode *child : entry.second) {
			child_measure += std::pow(child->radius(), exponent_gamma);
		}
		const float scale = std::max(parent_measure, child_measure);
		const float relative_error = scale <= 1.0e-8f
			? 0.0f
			: std::abs(parent_measure - child_measure) / scale;
		if (relative_error > relative_tolerance) {
			report.addIssue(
				BranchGraphDiagnosticCode::RadiusConservationViolation,
				"BranchNode '" + parent->identifier() +
				" violates the configured radius-conservation relation.");
		}
	}
	return report;
}
