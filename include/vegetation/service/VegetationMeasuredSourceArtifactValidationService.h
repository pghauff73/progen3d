#pragma once

#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

#include <string>
#include <vector>

class VegetationMeasuredSourceArtifactValidationService
{
public:
	std::vector<VegetationMeasuredSourceAdapterIssue> validate(
		const VegetationMeasuredSourceArtifact &artifact,
		const std::string &expected_media_type) const;
};
