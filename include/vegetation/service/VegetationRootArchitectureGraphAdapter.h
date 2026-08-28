#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

class VegetationRootArchitectureGraphAdapter
{
public:
	VegetationMeasuredSourceAdapterReport adapt(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationCalibrationSubjectScope &subject_scope) const;
};
