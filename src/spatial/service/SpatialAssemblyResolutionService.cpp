#include "spatial/service/SpatialAssemblyResolutionService.h"

#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/service/CollisionPositioningSolver.h"
#include "spatial/service/SpatialConstraintDependencyAnalyzer.h"
#include "spatial/service/SpatialConstraintResidualValidationService.h"
#include "spatial/service/SpatialPlacementTransaction.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

SpatialAssemblyResolutionResult SpatialAssemblyResolutionService::resolve(
	SpatialBuildingModel construction_model,
	SceneGenerationContext *scene_context,
	const SpatialPositioningSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	if (scene_context == nullptr) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::PlacementTransactionFailed,
			"Spatial assembly resolution requires a scene generation context."));
		return SpatialAssemblyResolutionResult(nullptr, std::move(report));
	}
	const SpatialConstraintDependencyReport dependency_report =
		SpatialConstraintDependencyAnalyzer().analyze(
			construction_model.constraintGraph(), construction_model.objects());
	report.append(dependency_report.validationReport());
	if (!dependency_report.isAcyclic()) {
		return SpatialAssemblyResolutionResult(nullptr, std::move(report));
	}

	std::map<SpatialConstraintId, const CollisionPositionConstraint *> constraints_by_id;
	for (const auto &constraint : construction_model.constraintGraph().constraints()) {
		if (!constraint || constraint->kind() != SpatialConstraintKind::CollisionPosition) {
			continue;
		}
		const auto *positioning =
			dynamic_cast<const CollisionPositionConstraint *>(constraint.get());
		if (positioning != nullptr) {
			constraints_by_id.emplace(positioning->constraintId(), positioning);
		}
	}

	SpatialPlacementTransaction transaction(construction_model, *scene_context);
	std::vector<const CollisionPositionConstraint *> resolved_constraints;
	for (const SpatialConstraintId &constraint_id :
	     dependency_report.topologicalConstraintOrder()) {
		const auto found = constraints_by_id.find(constraint_id);
		if (found == constraints_by_id.end()) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Dependency order references unavailable constraint '" +
					constraint_id.value() + "'."));
			transaction.discard();
			return SpatialAssemblyResolutionResult(nullptr, std::move(report));
		}
		const SpatialConstraintResolutionResult resolution =
			CollisionPositioningSolver().solve(
				*found->second,
				transaction.candidateModel(),
				*scene_context,
				&transaction.candidatePrimitiveInstances(),
				limits);
		if (!resolution.succeeded()) {
			report.append(resolution.validationReport());
			transaction.discard();
			return SpatialAssemblyResolutionResult(nullptr, std::move(report));
		}
		std::string staging_diagnostic;
		if (!transaction.stageConstraintResolution(resolution, &staging_diagnostic)) {
			report.addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::PlacementTransactionFailed,
				staging_diagnostic));
			transaction.discard();
			return SpatialAssemblyResolutionResult(nullptr, std::move(report));
		}
		resolved_constraints.push_back(found->second);
		for (const CollisionPositionConstraint *resolved_constraint : resolved_constraints) {
			const SpatialModelValidationReport residual_report =
				SpatialConstraintResidualValidationService().validate(
					*resolved_constraint,
					transaction.candidateModel(),
					*scene_context,
					&transaction.candidatePrimitiveInstances(),
					limits);
			report.append(residual_report);
			if (!residual_report.isValid()) {
				transaction.discard();
				return SpatialAssemblyResolutionResult(nullptr, std::move(report));
			}
		}
	}

	SpatialBuildingModel committed_model = construction_model;
	std::string commit_diagnostic;
	if (!transaction.commit(&committed_model, scene_context, &commit_diagnostic)) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::PlacementTransactionFailed,
			commit_diagnostic));
		return SpatialAssemblyResolutionResult(nullptr, std::move(report));
	}
	return SpatialAssemblyResolutionResult(
		std::make_shared<const SpatialBuildingModel>(std::move(committed_model)),
		std::move(report));
}
