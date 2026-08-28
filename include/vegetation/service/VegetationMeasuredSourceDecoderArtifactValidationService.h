#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeContext.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeReport.h"

#include <string>
#include <vector>

class VegetationMeasuredSourceDecoderArtifactValidationService
{
public:
	std::vector<VegetationMeasuredSourceDecodeIssue> validate(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationMeasuredSourceDecodeContext &context,
		const std::string &expected_media_type,
		const std::string &expected_source_schema_version) const;
};
