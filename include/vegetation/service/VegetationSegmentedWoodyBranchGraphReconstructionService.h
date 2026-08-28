#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstructionPolicy.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstructionReport.h"
#include "vegetation/model/VegetationWoodyPointSegmentation.h"

#include <string>

class VegetationSegmentedWoodyBranchGraphReconstructionService
{
public:
	VegetationWoodyBranchGraphReconstructionReport reconstruct(
		const std::string &reconstruction_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const VegetationPointCloudReconstructionJob &job,
		const VegetationWoodyPointSegmentation &segmentation,
		const VegetationWoodyBranchGraphReconstructionPolicy &policy) const;
};
