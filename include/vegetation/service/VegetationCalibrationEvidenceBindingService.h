#pragma once

#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationCalibrationEvidenceBindingReport.h"
#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

class VegetationCalibrationEvidenceBindingService
{
public:
	VegetationCalibrationEvidenceBindingReport bind(
		const BuildingVegetationObjectModel &object_model,
		const VegetationCalibrationEvidenceBundle &bundle) const;
};
