#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

#include <string>

class VegetationCalibrationEvidenceBundleSerializationService
{
public:
	std::string serialize(
		const VegetationCalibrationEvidenceBundle &bundle) const;
	std::string canonicalPayloadJson(
		const VegetationCalibrationEvidenceBundle &bundle) const;
};
