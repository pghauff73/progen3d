#pragma once

#include "vegetation/model/VegetationAutomatedWoodySegmentationEnsembleReport.h"
#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstructionPolicy.h"
#include "vegetation/model/VegetationWoodySegmentationParameterSet.h"
#include "vegetation/model/VegetationWoodySegmentationSensitivityPolicy.h"

#include <string>
#include <vector>

class VegetationAutomatedWoodySegmentationEnsembleService
{
public:
	VegetationAutomatedWoodySegmentationEnsembleReport reconstruct(
		const std::string &ensemble_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const VegetationPointCloudReconstructionJob &job,
		const std::vector<VegetationWoodySegmentationParameterSet>
			&parameter_sets,
		const VegetationWoodyBranchGraphReconstructionPolicy &graph_policy,
		const VegetationWoodySegmentationSensitivityPolicy &sensitivity_policy)
		const;
};
