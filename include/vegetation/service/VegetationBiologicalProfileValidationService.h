#pragma once

#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationBiologicalProfileValidationReport.h"

class VegetationBiologicalProfileValidationService
{
public:
	VegetationBiologicalProfileValidationReport validate(
		const BuildingVegetationObjectModel &object_model) const;
};
