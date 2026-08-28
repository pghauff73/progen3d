#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationPointCloudIngestionContext.h"
#include "vegetation/model/VegetationPointCloudIngestionReport.h"

#include <string>
#include <vector>

class VegetationPointCloudSourceArtifactValidationService
{
public:
	std::vector<VegetationPointCloudIngestionIssue> validate(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationPointCloudIngestionContext &context,
		const std::string &expected_media_type,
		const std::vector<std::string> &supported_source_schema_versions) const;
};
