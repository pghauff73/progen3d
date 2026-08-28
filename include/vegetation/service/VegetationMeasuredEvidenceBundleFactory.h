#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

#include <string>
#include <vector>

class VegetationMeasuredEvidenceBundleFactory
{
public:
	VegetationCalibrationEvidenceBundle create(
		const VegetationCalibrationSubjectScope &subject_scope,
		const VegetationMeasuredSourceArtifact &artifact,
		const std::string &adapter_identifier,
		std::vector<VegetationCalibrationMeasurement> measurements) const;
};
