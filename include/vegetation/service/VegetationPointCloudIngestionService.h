#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationPointCloudIngestionContext.h"
#include "vegetation/model/VegetationPointCloudIngestionReport.h"

class VegetationPointCloudIngestionService
{
public:
	VegetationPointCloudIngestionReport ingest(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationPointCloudIngestionContext &context) const;
};
