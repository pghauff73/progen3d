#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"
#include "vegetation/model/VegetationWoodySegmentationCandidateReport.h"
#include "vegetation/model/VegetationWoodySegmentationParameterSet.h"

#include <string>

class VegetationWoodyCoverSetSegmentationService
{
public:
	VegetationWoodySegmentationCandidateReport segment(
		const std::string &candidate_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const VegetationPointCloudReconstructionJob &job,
		const VegetationWoodySegmentationParameterSet &parameter_set) const;
};
