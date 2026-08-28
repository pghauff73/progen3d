#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionReport.h"
#include "vegetation/model/VegetationPointCloudReconstructionJob.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

class VegetationPointCloudReconstructionJobFactory
{
public:
	std::optional<VegetationPointCloudReconstructionJob> create(
		const std::string &job_identifier,
		const VegetationPointCloudDataset &dataset,
		const VegetationPointCloudReconstructionAdmissionReport &admission_report,
		const std::string &algorithm_identifier,
		std::size_t maximum_output_primitives,
		std::uint64_t deterministic_seed) const;
};
