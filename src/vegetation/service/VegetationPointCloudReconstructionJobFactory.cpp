#include "vegetation/service/VegetationPointCloudReconstructionJobFactory.h"

#include <optional>
#include <string>

std::optional<VegetationPointCloudReconstructionJob>
VegetationPointCloudReconstructionJobFactory::create(
	const std::string &job_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const std::string &algorithm_identifier,
	std::size_t maximum_output_primitives,
	std::uint64_t deterministic_seed) const
{
	if (!admission_report.admitted() || job_identifier.empty() ||
	    algorithm_identifier.empty() || maximum_output_primitives == 0u ||
	    admission_report.datasetIdentifier() != dataset.datasetIdentifier() ||
	    admission_report.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256()) {
		return std::nullopt;
	}
	return VegetationPointCloudReconstructionJob(
		"ProGen3D-VegetationPointCloudReconstructionJob-v1", job_identifier,
		admission_report.target(), dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), algorithm_identifier,
		maximum_output_primitives, deterministic_seed);
}
