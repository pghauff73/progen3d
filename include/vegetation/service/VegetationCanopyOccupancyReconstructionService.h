#pragma once

#include "vegetation/model/VegetationCanopyOccupancyReconstructionPolicy.h"
#include "vegetation/model/VegetationCanopyOccupancyReconstructionReport.h"
#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"

#include <string>

class VegetationCanopyOccupancyReconstructionService
{
public:
	VegetationCanopyOccupancyReconstructionReport reconstruct(
		const std::string &reconstruction_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const VegetationPointCloudReconstructionJob &job,
		const VegetationCanopyOccupancyReconstructionPolicy &policy) const;
};
