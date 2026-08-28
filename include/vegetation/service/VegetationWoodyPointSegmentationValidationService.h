#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationWoodyPointSegmentation.h"
#include "vegetation/model/VegetationWoodyPointSegmentationValidationReport.h"

class VegetationWoodyPointSegmentationValidationService
{
public:
	VegetationWoodyPointSegmentationValidationReport validate(
		const VegetationWoodyPointSegmentation &segmentation,
		const VegetationPointCloudDataset &dataset) const;
};
