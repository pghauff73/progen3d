#pragma once

#include <cstdint>

class BuildingClassificationModel;
class BuildingEvidenceLedger;
class BuildingFunctionModel;
class BuildingRequirementModel;
class BuildingRelationshipAssertionModel;
class BuildingScenarioModel;
class BuildingServiceModel;
class SpatialBuildingModel;

class SmallModernBuildingDeterministicHashService {
public:
	std::uint64_t calculate(
		const SpatialBuildingModel &spatial_model,
		const BuildingClassificationModel &classification_model,
		const BuildingFunctionModel &function_model,
		const BuildingServiceModel &service_model,
		const BuildingRelationshipAssertionModel &relationship_assertion_model,
		const BuildingRequirementModel &requirement_model,
		const BuildingScenarioModel &scenario_model,
		const BuildingEvidenceLedger &evidence_ledger) const;
};
