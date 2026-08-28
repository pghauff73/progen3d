#include "building/service/BuildingFunctionCoverageEvaluationService.h"

#include "spatial/model/SpatialBuildingModel.h"

#include <map>
#include <set>

namespace {

bool visit_function(
	const BuildingFunctionId &function_id,
	const std::multimap<BuildingFunctionId, BuildingFunctionId> &dependencies,
	std::set<BuildingFunctionId> *visiting,
	std::set<BuildingFunctionId> *visited)
{
	if (visited->find(function_id) != visited->end()) return true;
	if (!visiting->insert(function_id).second) return false;
	const auto range = dependencies.equal_range(function_id);
	for (auto dependency = range.first; dependency != range.second; ++dependency) {
		if (!visit_function(dependency->second, dependencies, visiting, visited)) return false;
	}
	visiting->erase(function_id);
	visited->insert(function_id);
	return true;
}

} // namespace

BuildingModelValidationReport BuildingFunctionCoverageEvaluationService::validate(
	const BuildingFunctionModel &function_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (function_model.functions().size() > limits.maximum_functions ||
	    function_model.allocations().size() > limits.maximum_allocations ||
	    function_model.dependencies().size() > limits.maximum_function_dependencies) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building function model exceeds a configured safety ceiling."));
	}
	std::set<BuildingFunctionId> function_ids;
	for (const BuildingFunction &function : function_model.functions()) {
		if (function.functionId().empty() || function.purpose().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidIdentifier,
				"Building function ID and purpose must not be empty."));
		}
		if (!function_ids.insert(function.functionId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateFunctionId,
				"Duplicate building function ID '" + function.functionId().value() + "'."));
		}
	}
	std::set<BuildingFunctionAllocationId> allocation_ids;
	std::map<BuildingFunctionId, std::size_t> allocation_counts;
	for (const BuildingFunctionAllocationRelationship &allocation :
	     function_model.allocations()) {
		if (!allocation_ids.insert(allocation.allocationId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateFunctionAllocationId,
				"Duplicate function allocation ID '" +
					allocation.allocationId().value() + "'."));
		}
		if (function_ids.find(allocation.functionId()) == function_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedFunction,
				"Function allocation references undefined function '" +
					allocation.functionId().value() + "'."));
		}
		if (spatial_model.objects().find(allocation.objectId()) == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedFunctionAllocationObject,
				"Function allocation references an undefined spatial object.",
				{allocation.objectId()}));
		}
		++allocation_counts[allocation.functionId()];
	}
	std::multimap<BuildingFunctionId, BuildingFunctionId> dependencies;
	for (const BuildingFunctionalDependencyRelationship &dependency :
	     function_model.dependencies()) {
		if (function_ids.find(dependency.prerequisiteFunctionId()) == function_ids.end() ||
		    function_ids.find(dependency.dependentFunctionId()) == function_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedFunction,
				"Functional dependency references an undefined function."));
		} else {
			dependencies.emplace(
				dependency.dependentFunctionId(), dependency.prerequisiteFunctionId());
		}
	}
	std::set<BuildingFunctionId> visiting;
	std::set<BuildingFunctionId> visited;
	for (const BuildingFunctionId &function_id : function_ids) {
		if (!visit_function(function_id, dependencies, &visiting, &visited)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::FunctionalDependencyCycle,
				"Building function dependency graph contains a cycle."));
			break;
		}
	}
	for (const BuildingFunction &function : function_model.functions()) {
		if (function.criticality() == BuildingFunctionCriticality::Hard &&
		    allocation_counts[function.functionId()] == 0) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UncoveredHardFunction,
				"Hard building function '" + function.functionId().value() +
					"' has no object allocation."));
		}
	}
	return report;
}

std::vector<BuildingFunctionCoverageResult>
BuildingFunctionCoverageEvaluationService::evaluate(
	const BuildingFunctionModel &function_model) const
{
	std::map<BuildingFunctionId, std::size_t> counts;
	for (const BuildingFunctionAllocationRelationship &allocation :
	     function_model.allocations()) {
		++counts[allocation.functionId()];
	}
	std::vector<BuildingFunctionCoverageResult> results;
	for (const BuildingFunction &function : function_model.functions()) {
		const std::size_t count = counts[function.functionId()];
		results.emplace_back(function.functionId(), count, count > 0);
	}
	return results;
}
