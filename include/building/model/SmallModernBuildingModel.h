#pragma once

#include "building/model/BuildingClassificationModel.h"
#include "building/model/BuildingEvidenceLedger.h"
#include "building/model/BuildingFunctionModel.h"
#include "building/model/BuildingRequirementModel.h"
#include "building/model/BuildingRelationshipAssertionModel.h"
#include "building/model/BuildingScenarioModel.h"
#include "building/model/BuildingServiceModel.h"

#include <cstdint>
#include <memory>
#include <utility>

class SpatialBuildingModel;

class SmallModernBuildingModel {
public:
	SmallModernBuildingModel(
		std::shared_ptr<const SpatialBuildingModel> spatial_model,
		BuildingClassificationModel classification_model,
		BuildingFunctionModel function_model,
		BuildingServiceModel service_model,
		BuildingRelationshipAssertionModel relationship_assertion_model,
		BuildingRequirementModel requirement_model,
		BuildingScenarioModel scenario_model,
		BuildingEvidenceLedger evidence_ledger,
		std::uint64_t model_hash)
		: spatial_model_(std::move(spatial_model)),
		  classification_model_(std::move(classification_model)),
		  function_model_(std::move(function_model)),
		  service_model_(std::move(service_model)),
		  relationship_assertion_model_(std::move(relationship_assertion_model)),
		  requirement_model_(std::move(requirement_model)),
		  scenario_model_(std::move(scenario_model)),
		  evidence_ledger_(std::move(evidence_ledger)),
		  model_hash_(model_hash) {}

	const std::shared_ptr<const SpatialBuildingModel> &spatialModel() const
	{
		return spatial_model_;
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
	const BuildingRequirementModel &requirementModel() const
	{
		return requirement_model_;
	}
	const BuildingScenarioModel &scenarioModel() const { return scenario_model_; }
	const BuildingEvidenceLedger &evidenceLedger() const { return evidence_ledger_; }
	std::uint64_t modelHash() const { return model_hash_; }

private:
	std::shared_ptr<const SpatialBuildingModel> spatial_model_;
	BuildingClassificationModel classification_model_;
	BuildingFunctionModel function_model_;
	BuildingServiceModel service_model_;
	BuildingRelationshipAssertionModel relationship_assertion_model_;
	BuildingRequirementModel requirement_model_;
	BuildingScenarioModel scenario_model_;
	BuildingEvidenceLedger evidence_ledger_;
	std::uint64_t model_hash_ = 0;
};
