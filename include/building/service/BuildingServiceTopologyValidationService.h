#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"
#include "building/model/BuildingServiceContinuityResult.h"
#include "building/model/BuildingServiceModel.h"

class SpatialBuildingModel;

class BuildingServiceTopologyValidationService {
public:
	BuildingModelValidationReport validate(
		const BuildingServiceModel &service_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;

	BuildingServiceContinuityResult evaluateContinuity(
		const BuildingServiceModel &service_model,
		const BuildingServicePortId &source_port_id,
		const BuildingServicePortId &target_port_id) const;
};
