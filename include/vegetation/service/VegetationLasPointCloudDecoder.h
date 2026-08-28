#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationPointCloudIngestionContext.h"
#include "vegetation/model/VegetationPointCloudIngestionReport.h"

class VegetationLasPointCloudDecoder
{
public:
	VegetationPointCloudIngestionReport decode(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationPointCloudIngestionContext &context) const;
};
