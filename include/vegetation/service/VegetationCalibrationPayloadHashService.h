#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

#include <string>

class VegetationCalibrationPayloadHashService
{
public:
	std::string calculateCanonicalPayloadSha256(
		const VegetationCalibrationEvidenceBundle &bundle) const;
	VegetationCalibrationEvidenceBundle attachCanonicalPayloadSha256(
		const VegetationCalibrationEvidenceBundle &bundle) const;
};
