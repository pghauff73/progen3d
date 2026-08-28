#include "building/service/SmallModernBuildingModelConstructionService.h"

#include "building/model/SmallModernBuildingModel.h"
#include "building/service/BuildingEvidenceHashService.h"
#include "building/service/BuildingObjectSemanticProfileHashService.h"
#include "building/service/SmallModernBuildingDeterministicHashService.h"
#include "building/service/SmallModernBuildingModelValidationService.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <chrono>
#include <memory>

namespace {

double elapsed_milliseconds(
	const std::chrono::steady_clock::time_point &begin,
	const std::chrono::steady_clock::time_point &end)
{
	return std::chrono::duration<double, std::milli>(end - begin).count();
}

std::size_t peak_record_count(
	const SmallModernBuildingModelConstructionRequest &request)
{
	return std::max({
		request.classificationModel().concepts().size(),
		request.classificationModel().roles().size(),
		request.classificationModel().roleAssignments().size(),
		request.classificationModel().objectProfiles().size(),
		request.functionModel().functions().size(),
		request.functionModel().allocations().size(),
		request.serviceModel().ports().size(),
		request.serviceModel().flows().size(),
		request.relationshipAssertionModel().assertions().size(),
		request.requirementModel().requirements().size(),
		request.requirementModel().evaluationRecords().size(),
		request.scenarioModel().participations().size(),
		request.evidenceLedger().records().size()});
}

} // namespace

SmallModernBuildingModelConstructionResult
SmallModernBuildingModelConstructionService::construct(
	const SpatialBuildingModel &spatial_model,
	const SmallModernBuildingModelConstructionRequest &request,
	const BuildingModelSafetyLimits &limits) const
{
	return construct(
		std::shared_ptr<const SpatialBuildingModel>(
			&spatial_model, [](const SpatialBuildingModel *) {}),
		request, limits);
}

SmallModernBuildingModelConstructionResult
SmallModernBuildingModelConstructionService::construct(
	std::shared_ptr<const SpatialBuildingModel> spatial_model,
	const SmallModernBuildingModelConstructionRequest &request,
	const BuildingModelSafetyLimits &limits) const
{
	const auto total_begin = std::chrono::steady_clock::now();
	SmallModernBuildingModelConstructionMetrics metrics;
	metrics.setPeakRecordCount(peak_record_count(request));
	if (!spatial_model) {
		BuildingModelValidationReport report;
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::UndefinedProfileObject,
			"Small modern building construction requires a spatial model."));
		return SmallModernBuildingModelConstructionResult(
			nullptr, std::move(report), metrics);
	}

	const auto normalization_begin = std::chrono::steady_clock::now();
	const BuildingEvidenceHashService evidence_hash_service;
	std::vector<BuildingEvidenceRecord> evidence_records;
	evidence_records.reserve(request.evidenceLedger().records().size());
	for (const BuildingEvidenceRecord &record : request.evidenceLedger().records()) {
		evidence_records.push_back(evidence_hash_service.attachCalculatedHash(record));
	}
	BuildingEvidenceLedger evidence_ledger(std::move(evidence_records));

	const BuildingObjectSemanticProfileHashService profile_hash_service;
	std::vector<BuildingObjectSemanticProfile> profiles;
	profiles.reserve(request.classificationModel().objectProfiles().size());
	for (const BuildingObjectSemanticProfile &profile :
	     request.classificationModel().objectProfiles()) {
		profiles.push_back(profile_hash_service.attachCalculatedHash(profile));
	}
	BuildingClassificationModel classification_model(
		request.classificationModel().concepts(),
		request.classificationModel().roles(),
		request.classificationModel().roleAssignments(),
		std::move(profiles));

	std::vector<BuildingRequirementEvaluationRecord> evaluations;
	evaluations.reserve(request.requirementModel().evaluationRecords().size());
	for (const BuildingRequirementEvaluationRecord &record :
	     request.requirementModel().evaluationRecords()) {
		evaluations.push_back(evidence_hash_service.attachCalculatedHash(record));
	}
	BuildingRequirementModel requirement_model(
		request.requirementModel().requirements(), std::move(evaluations));
	const auto normalization_end = std::chrono::steady_clock::now();
	metrics.setNormalizationMilliseconds(
		elapsed_milliseconds(normalization_begin, normalization_end));

	SmallModernBuildingModelConstructionRequest normalized_request(
		request.sourceSpatialSchema(), request.sourceRootObjectId(),
		request.expectedObjectIds(), classification_model, request.functionModel(),
		request.serviceModel(), request.relationshipAssertionModel(), requirement_model,
		request.scenarioModel(), evidence_ledger);
	const auto validation_begin = std::chrono::steady_clock::now();
	double requirement_evaluation_milliseconds = 0.0;
	BuildingModelValidationReport report =
		SmallModernBuildingModelValidationService().validate(
			*spatial_model, normalized_request, limits,
			&requirement_evaluation_milliseconds);
	const auto validation_end = std::chrono::steady_clock::now();
	metrics.setValidationMilliseconds(
		elapsed_milliseconds(validation_begin, validation_end));
	metrics.setRequirementEvaluationMilliseconds(requirement_evaluation_milliseconds);
	if (!report.isValid()) {
		metrics.setTotalMilliseconds(elapsed_milliseconds(
			total_begin, std::chrono::steady_clock::now()));
		return SmallModernBuildingModelConstructionResult(
			nullptr, std::move(report), metrics);
	}

	const auto hashing_begin = std::chrono::steady_clock::now();
	const std::uint64_t aggregate_hash =
		SmallModernBuildingDeterministicHashService().calculate(
			*spatial_model, classification_model, request.functionModel(),
			request.serviceModel(), request.relationshipAssertionModel(), requirement_model,
			request.scenarioModel(), evidence_ledger);
	const auto hashing_end = std::chrono::steady_clock::now();
	metrics.setHashingMilliseconds(elapsed_milliseconds(hashing_begin, hashing_end));
	if (aggregate_hash == 0) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::AggregateHashMissing,
			"Small modern building aggregate hash is missing."));
		metrics.setTotalMilliseconds(elapsed_milliseconds(total_begin, hashing_end));
		return SmallModernBuildingModelConstructionResult(
			nullptr, std::move(report), metrics);
	}

	auto model = std::make_shared<const SmallModernBuildingModel>(
		std::move(spatial_model), std::move(classification_model),
		request.functionModel(), request.serviceModel(), request.relationshipAssertionModel(),
		std::move(requirement_model), request.scenarioModel(), std::move(evidence_ledger),
		aggregate_hash);
	metrics.setTotalMilliseconds(elapsed_milliseconds(total_begin, hashing_end));
	return SmallModernBuildingModelConstructionResult(
		std::move(model), std::move(report), metrics);
}
