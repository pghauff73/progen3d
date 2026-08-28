#include "spatial/service/SpatialConstraintDependencyAnalyzer.h"

#include "spatial/model/CollisionPositionConstraint.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace {

struct ConstraintDependency {
	SpatialConstraintId constraint_id;
	SpatialObjectId target_object_id;
	SpatialObjectId moving_object_id;
	int priority = 0;
};

std::string format_cycle(const std::vector<SpatialObjectId> &path)
{
	std::ostringstream text;
	text << "Spatial constraint cycle:";
	for (const SpatialObjectId &object_id : path) text << "\n-> " << object_id.value();
	return text.str();
}

std::vector<SpatialObjectId> find_dependency_cycle(
	const std::vector<SpatialObjectId> &ordered_objects,
	const std::map<SpatialObjectId, std::vector<SpatialObjectId>> &edges)
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
		const auto outgoing = edges.find(object_id);
		if (outgoing != edges.end()) {
			for (const SpatialObjectId &dependent : outgoing->second) {
				if (states[dependent] == VisitState::Visiting) {
					const std::size_t start = stack_indices[dependent];
					cycle.assign(stack.begin() + static_cast<std::ptrdiff_t>(start), stack.end());
					cycle.push_back(dependent);
					return true;
				}
				if (states[dependent] == VisitState::Unvisited && self(self, dependent)) {
					return true;
				}
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

SpatialConstraintDependencyReport SpatialConstraintDependencyAnalyzer::analyze(
	const SpatialConstraintGraph &constraint_graph,
	const SpatialObjectRegistry &objects,
	const SpatialModelSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	std::vector<SpatialObjectId> ordered_objects;
	ordered_objects.reserve(objects.size());
	for (const SpatialBuildingObject &object : objects.objects()) {
		ordered_objects.push_back(object.identity().objectId());
	}
	std::sort(ordered_objects.begin(), ordered_objects.end());

	std::map<SpatialObjectId, std::vector<SpatialObjectId>> edges;
	std::map<SpatialObjectId, std::size_t> indegrees;
	for (const SpatialObjectId &object_id : ordered_objects) indegrees[object_id] = 0u;

	std::vector<ConstraintDependency> dependencies;
	for (const auto &constraint : constraint_graph.constraints()) {
		if (!constraint) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial constraint graph contains a null constraint."));
			continue;
		}
		if (constraint->kind() != SpatialConstraintKind::CollisionPosition) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UnsupportedConstraint,
				"Spatial constraint '" + constraint->constraintId().value() +
					"' does not expose a P0 dependency."));
			continue;
		}
		const auto *positioning = dynamic_cast<const CollisionPositionConstraint *>(constraint.get());
		if (positioning == nullptr) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial collision-position constraint has an invalid runtime type."));
			continue;
		}
		if (objects.find(positioning->movingObjectId()) == nullptr ||
		    objects.find(positioning->targetObjectId()) == nullptr) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UndefinedConstraintEndpoint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' references an undefined object.",
				{positioning->targetObjectId(), positioning->movingObjectId()}));
			continue;
		}
		dependencies.push_back({positioning->constraintId(),
		                        positioning->targetObjectId(),
		                        positioning->movingObjectId(),
		                        positioning->priority()});
		edges[positioning->targetObjectId()].push_back(positioning->movingObjectId());
	}

	for (auto &entry : edges) {
		std::sort(entry.second.begin(), entry.second.end());
		entry.second.erase(std::unique(entry.second.begin(), entry.second.end()), entry.second.end());
		for (const SpatialObjectId &dependent : entry.second) ++indegrees[dependent];
	}

	const std::vector<SpatialObjectId> cycle = find_dependency_cycle(ordered_objects, edges);
	if (!cycle.empty()) {
		std::vector<SpatialObjectId> bounded_cycle = cycle;
		if (bounded_cycle.size() > limits.maximum_cycle_diagnostic_length) {
			bounded_cycle.resize(limits.maximum_cycle_diagnostic_length);
		}
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::ConstraintCycle,
			format_cycle(bounded_cycle),
			bounded_cycle));
		return SpatialConstraintDependencyReport({}, {}, bounded_cycle, std::move(report));
	}

	std::priority_queue<SpatialObjectId,
	                    std::vector<SpatialObjectId>,
	                    std::greater<SpatialObjectId>> ready;
	for (const auto &entry : indegrees) {
		if (entry.second == 0u) ready.push(entry.first);
	}
	std::vector<SpatialObjectId> topological_objects;
	std::map<SpatialObjectId, std::size_t> topological_indices;
	std::map<SpatialObjectId, std::size_t> depths;
	while (!ready.empty()) {
		const SpatialObjectId current = ready.top();
		ready.pop();
		topological_indices[current] = topological_objects.size();
		topological_objects.push_back(current);
		const auto outgoing = edges.find(current);
		if (outgoing == edges.end()) continue;
		for (const SpatialObjectId &dependent : outgoing->second) {
			depths[dependent] = std::max(depths[dependent], depths[current] + 1u);
			if (depths[dependent] > limits.maximum_constraint_dependency_depth) {
				report.addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::ConstraintDependencyDepthExceeded,
					"Spatial constraint dependency depth exceeds the configured ceiling at object '" +
						dependent.value() + "'.",
					{dependent}));
			}
			if (--indegrees[dependent] == 0u) ready.push(dependent);
		}
	}

	std::sort(dependencies.begin(), dependencies.end(),
	          [&](const ConstraintDependency &left, const ConstraintDependency &right) {
		          const std::size_t left_index = topological_indices[left.moving_object_id];
		          const std::size_t right_index = topological_indices[right.moving_object_id];
		          if (left_index != right_index) return left_index < right_index;
		          if (left.priority != right.priority) return left.priority > right.priority;
		          return left.constraint_id < right.constraint_id;
	          });
	std::vector<SpatialConstraintId> topological_constraints;
	topological_constraints.reserve(dependencies.size());
	for (const ConstraintDependency &dependency : dependencies) {
		topological_constraints.push_back(dependency.constraint_id);
	}
	return SpatialConstraintDependencyReport(
		std::move(topological_objects),
		std::move(topological_constraints),
		{},
		std::move(report));
}
