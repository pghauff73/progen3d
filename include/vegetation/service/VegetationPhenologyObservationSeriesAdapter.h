#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

class VegetationPhenologyObservationSeriesAdapter
{
public:
	VegetationMeasuredSourceAdapterReport adapt(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationCalibrationSubjectScope &subject_scope) const;
};
