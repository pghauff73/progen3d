#pragma once

#include "building/model/BuildingClassificationModel.h"
#include "building/model/BuildingEvidenceLedger.h"
#include "building/model/BuildingFunctionModel.h"
#include "building/model/BuildingRequirementModel.h"
#include "building/model/BuildingRelationshipAssertionModel.h"
#include "building/model/BuildingScenarioModel.h"
#include "building/model/BuildingServiceModel.h"
#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>
#include <vector>

class SmallModernBuildingModelConstructionRequest {
public:
	SmallModernBuildingModelConstructionRequest(
		std::string source_spatial_schema,
		SpatialObjectId source_root_object_id,
		std::vector<SpatialObjectId> expected_object_ids,
		BuildingClassificationModel classification_model,
		BuildingFunctionModel function_model,
		BuildingServiceModel service_model,
		BuildingRelationshipAssertionModel relationship_assertion_model,
		BuildingRequirementModel requirement_model,
		BuildingScenarioModel scenario_model,
		BuildingEvidenceLedger evidence_ledger)
		: source_spatial_schema_(std::move(source_spatial_schema)),
		  source_root_object_id_(std::move(source_root_object_id)),
		  expected_object_ids_(std::move(expected_object_ids)),
		  classification_model_(std::move(classification_model)),
		  function_model_(std::move(function_model)),
		  service_model_(std::move(service_model)),
		  relationship_assertion_model_(std::move(relationship_assertion_model)),
		  requirement_model_(std::move(requirement_model)),
		  scenario_model_(std::move(scenario_model)),
		  evidence_ledger_(std::move(evidence_ledger)) {}

	const std::string &sourceSpatialSchema() const { return source_spatial_schema_; }
	const SpatialObjectId &sourceRootObjectId() const { return source_root_object_id_; }
	const std::vector<SpatialObjectId> &expectedObjectIds() const
	{
		return expected_object_ids_;
	}
	const BuildingClassificationModel &classificationModel() const
	{
		return classification_model_;
	}
	const BuildingFunctionModel &functionModel() const { return function_model_; }
	const BuildingServiceModel &serviceModel() const { return service_model_; }
	const BuildingRelationshipAssertionModel &relationshipAssertionModel() const
	{
		return relationship_assertion_model_;
	}
	const BuildingRequirementModel &requirementModel() const { return requirement_model_; }
	const BuildingScenarioModel &scenarioModel() const { return scenario_model_; }
	const BuildingEvidenceLedger &evidenceLedger() const { return evidence_ledger_; }

private:
	std::string source_spatial_schema_;
	SpatialObjectId source_root_object_id_;
	std::vector<SpatialObjectId> expected_object_ids_;
	BuildingClassificationModel classification_model_;
	BuildingFunctionModel function_model_;
	BuildingServiceModel service_model_;
	BuildingRelationshipAssertionModel relationship_assertion_model_;
	BuildingRequirementModel requirement_model_;
	BuildingScenarioModel scenario_model_;
	BuildingEvidenceLedger evidence_ledger_;
};
