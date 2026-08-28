#pragma once

#include "vehicle/mcsmv2/model/ModernCarParameterDependencyNode.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

class ModernCarParameterDependencyGraph
{
public:
	ModernCarParameterDependencyGraph(
		std::string identifier,
		std::vector<ModernCarParameterDependencyNode> ordered_nodes)
		: identifier_(std::move(identifier)),
		  ordered_nodes_(std::move(ordered_nodes))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<ModernCarParameterDependencyNode> &orderedNodes() const
	{
		return ordered_nodes_;
	}

	const ModernCarParameterDependencyNode *findNode(
		const std::string &identifier) const
	{
		for (const ModernCarParameterDependencyNode &node : ordered_nodes_) {
			if (node.identifier() == identifier) return &node;
		}
		return nullptr;
	}

	bool hasValidEvaluationOrder() const
	{
		std::set<std::string> visited;
		for (const ModernCarParameterDependencyNode &node : ordered_nodes_) {
			if (node.identifier().empty() || visited.count(node.identifier()) != 0u) {
				return false;
			}
			for (const std::string &dependency : node.dependencies()) {
				if (visited.count(dependency) == 0u) return false;
			}
			visited.insert(node.identifier());
		}
		return true;
	}

private:
	std::string identifier_;
	std::vector<ModernCarParameterDependencyNode> ordered_nodes_;
};
