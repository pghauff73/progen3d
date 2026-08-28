#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionPolicy.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"

class VegetationPointCloudReconstructionAdmissionService
{
public:
	VegetationPointCloudReconstructionAdmissionReport evaluate(
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionPolicy &policy) const;
};
