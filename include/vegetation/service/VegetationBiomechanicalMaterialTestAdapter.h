#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

class VegetationBiomechanicalMaterialTestAdapter
{
public:
	VegetationMeasuredSourceAdapterReport adapt(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationCalibrationSubjectScope &subject_scope) const;
};
