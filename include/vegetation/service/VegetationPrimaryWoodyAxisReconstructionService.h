#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"
#include "vegetation/model/VegetationWoodyAxisReconstructionPolicy.h"
#include "vegetation/model/VegetationWoodyAxisReconstructionReport.h"

#include <string>

class VegetationPrimaryWoodyAxisReconstructionService
{
public:
	VegetationWoodyAxisReconstructionReport reconstruct(
		const std::string &reconstruction_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const VegetationPointCloudReconstructionJob &job,
		const VegetationWoodyAxisReconstructionPolicy &policy) const;
};
