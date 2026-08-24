#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"
#include "building/model/BuildingRequirementModel.h"

class BuildingClassificationModel;
class BuildingEvidenceLedger;
class BuildingFunctionModel;
class BuildingScenarioModel;
class BuildingServiceModel;
class SpatialBuildingModel;

class BuildingRequirementEvaluationService {
public:
	BuildingModelValidationReport validateFinalEvaluations(
		const BuildingRequirementModel &requirement_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingClassificationModel &classification_model,
		const BuildingFunctionModel &function_model,
		const BuildingServiceModel &service_model,
		const BuildingScenarioModel &scenario_model,
		const BuildingEvidenceLedger &evidence_ledger,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
};
