#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundleParsingResult.h"

#include <string>

class VegetationCalibrationEvidenceBundleParsingService
{
public:
	VegetationCalibrationEvidenceBundleParsingResult parse(
		const std::string &json_text) const;
};
