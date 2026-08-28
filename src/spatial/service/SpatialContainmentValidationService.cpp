#include "spatial/service/SpatialContainmentValidationService.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace {

std::string format_cycle(const std::vector<SpatialObjectId> &path)
{
	std::ostringstream text;
	text << "Spatial containment cycle:";
	for (const SpatialObjectId &object_id : path) {
		text << "\n-> " << object_id.value();
	}
	return text.str();
}

std::vector<SpatialObjectId> find_cycle(
	const std::vector<SpatialObjectId> &ordered_objects,
	const std::map<SpatialObjectId, std::vector<SpatialObjectId>> &children_by_parent)
{
	enum class VisitState { Unvisited, Visiting, Complete };
	std::map<SpatialObjectId, VisitState> states;
	std::vector<SpatialObjectId> stack;
	std::map<SpatialObjectId, std::size_t> stack_indices;
	std::vector<SpatialObjectId> cycle;

	const auto visit = [&](const auto &self, const SpatialObjectId &object_id) -> bool {
		states[object_id] = VisitState::Visiting;
		stack_indices[object_id] = stack.size();
		stack.push_back(object_id);
		const auto children = children_by_parent.find(object_id);
		if (children != children_by_parent.end()) {
			for (const SpatialObjectId &child : children->second) {
				const VisitState child_state = states[child];
				if (child_state == VisitState::Visiting) {
					const std::size_t start = stack_indices[child];
					cycle.assign(stack.begin() + static_cast<std::ptrdiff_t>(start), stack.end());
					cycle.push_back(child);
					return true;
				}
				if (child_state == VisitState::Unvisited && self(self, child)) return true;
			}
		}
		stack.pop_back();
		stack_indices.erase(object_id);
		states[object_id] = VisitState::Complete;
		return false;
	};

	for (const SpatialObjectId &object_id : ordered_objects) {
		if (states[object_id] == VisitState::Unvisited && visit(visit, object_id)) break;
	}
	return cycle;
}

} // namespace

SpatialModelValidationReport SpatialContainmentValidationService::validate(
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialObjectId &root_object_id,
	const std::vector<SpatialContainmentRelationship> &relationships,
	const SpatialModelSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	std::set<SpatialObjectId> object_ids;
	std::vector<SpatialObjectId> ordered_object_ids;
	ordered_object_ids.reserve(objects.size());
	for (const SpatialBuildingObject &object : objects) {
		object_ids.insert(object.identity().objectId());
		ordered_object_ids.push_back(object.identity().objectId());
	}
	std::sort(ordered_object_ids.begin(), ordered_object_ids.end());
	ordered_object_ids.erase(
		std::unique(ordered_object_ids.begin(), ordered_object_ids.end()),
		ordered_object_ids.end());

	if (root_object_id.empty() || object_ids.find(root_object_id) == object_ids.end()) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::MissingRootObject,
			"Spatial model root object is undefined."));
	}

	std::map<SpatialObjectId, std::vector<SpatialObjectId>> parents_by_child;
	std::map<SpatialObjectId, std::vector<SpatialObjectId>> children_by_parent;
	for (const SpatialContainmentRelationship &relationship : relationships) {
		const SpatialObjectId &parent = relationship.containerId();
		const SpatialObjectId &child = relationship.containedObjectId();
		if (object_ids.find(parent) == object_ids.end()) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UndefinedContainer,
				"Spatial container '" + parent.value() + "' is not defined.",
				{parent, child}));
		}
		if (object_ids.find(child) == object_ids.end()) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::MissingContainer,
				"Contained spatial object '" + child.value() + "' is not defined.",
				{parent, child}));
		}
		if (parent == child) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::SelfContainment,
				"Spatial object '" + child.value() + "' cannot contain itself.",
				{child, child}));
		}
		parents_by_child[child].push_back(parent);
		children_by_parent[parent].push_back(child);
	}

	for (auto &entry : children_by_parent) {
		std::sort(entry.second.begin(), entry.second.end());
		entry.second.erase(std::unique(entry.second.begin(), entry.second.end()), entry.second.end());
	}

	std::size_t root_count = 0;
	for (const SpatialObjectId &object_id : ordered_object_ids) {
		const auto parents = parents_by_child.find(object_id);
		const std::size_t parent_count =
			parents == parents_by_child.end() ? 0u : parents->second.size();
		if (parent_count == 0u) {
			++root_count;
			if (object_id != root_object_id) {
				report.addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::MissingContainer,
					"Non-root spatial object '" + object_id.value() +
						"' does not have a container.",
					{object_id}));
			}
		} else if (object_id == root_object_id) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::MultipleRootObjects,
				"Spatial root object '" + object_id.value() +
					"' must not have a container.",
				{object_id}));
		} else if (parent_count > 1u) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::MultipleContainers,
				"Spatial object '" + object_id.value() +
					"' has more than one container.",
				{object_id}));
		}
	}
	if (root_count != 1u) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::MultipleRootObjects,
			"Spatial containment requires exactly one root object; found " +
				std::to_string(root_count) + "."));
	}

	const std::vector<SpatialObjectId> cycle = find_cycle(ordered_object_ids, children_by_parent);
	if (!cycle.empty()) {
		std::vector<SpatialObjectId> bounded_cycle = cycle;
		if (bounded_cycle.size() > limits.maximum_cycle_diagnostic_length) {
			bounded_cycle.resize(limits.maximum_cycle_diagnostic_length);
		}
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::ContainmentCycle,
			format_cycle(bounded_cycle),
			bounded_cycle));
	}

	if (cycle.empty() && object_ids.find(root_object_id) != object_ids.end()) {
		std::vector<std::pair<SpatialObjectId, std::size_t>> pending{
			{root_object_id, 1u}};
		while (!pending.empty()) {
			const auto current = pending.back();
			pending.pop_back();
			if (current.second > limits.maximum_containment_depth) {
				report.addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::ContainmentDepthExceeded,
					"Spatial containment depth exceeds the configured ceiling at object '" +
						current.first.value() + "'.",
					{current.first}));
				break;
			}
			const auto children = children_by_parent.find(current.first);
			if (children == children_by_parent.end()) continue;
			for (auto child = children->second.rbegin(); child != children->second.rend(); ++child) {
				pending.push_back({*child, current.second + 1u});
			}
		}
	}

	for (const SpatialBuildingObject &object : objects) {
		const SpatialObjectId &object_id = object.identity().objectId();
		const SpatialFrameReference &parent_frame = object.frameState().parentFrame();
		if (object_id == root_object_id) {
			if (!parent_frame.isRoot()) {
				report.addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::FrameParentMismatch,
					"Spatial root object '" + object_id.value() +
						"' must use the root frame.",
					{object_id}));
			}
			continue;
		}
		const auto parents = parents_by_child.find(object_id);
		if (parents == parents_by_child.end() || parents->second.size() != 1u) continue;
		if (parent_frame.isRoot() || parent_frame.objectId() != parents->second.front()) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::FrameParentMismatch,
				"Spatial object '" + object_id.value() +
					"' frame parent does not match its containment parent '" +
					parents->second.front().value() + "'.",
				{parents->second.front(), object_id}));
		}
	}
	return report;
}
