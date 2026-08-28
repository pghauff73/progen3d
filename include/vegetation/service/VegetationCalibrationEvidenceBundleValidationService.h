#pragma once

#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationCalibrationEvidenceBundleValidationReport.h"

class VegetationCalibrationEvidenceBundleValidationService
{
public:
	VegetationCalibrationEvidenceBundleValidationReport validate(
		const BuildingVegetationObjectModel &object_model,
		const VegetationCalibrationEvidenceBundle &bundle) const;
};
