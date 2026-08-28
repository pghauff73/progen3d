#include "vegetation/service/VegetationMeasuredEvidenceBundleFactory.h"

#include "vegetation/service/VegetationCalibrationPayloadHashService.h"

#include <string>
#include <utility>
#include <vector>

VegetationCalibrationEvidenceBundle
VegetationMeasuredEvidenceBundleFactory::create(
	const VegetationCalibrationSubjectScope &subject_scope,
	const VegetationMeasuredSourceArtifact &artifact,
	const std::string &adapter_identifier,
	std::vector<VegetationCalibrationMeasurement> measurements) const
{
	const VegetationCalibrationEvidenceBundle unsigned_bundle(
		"ProGen3D-VegetationCalibrationEvidenceBundle-v1",
		"bundle:" + artifact.sourceIdentifier() + ":" + adapter_identifier,
		subject_scope,
		"LocalPlantXYZ-ZUp",
		"SI",
		artifact.acquisitionMethod(),
		artifact.uncertaintyStatement(),
		{
			VegetationCalibrationSourceReference(
				artifact.sourceIdentifier(),
				artifact.sourceCitation(),
				artifact.sourceLocator(),
				artifact.payloadSha256()),
		},
		std::move(measurements),
		std::string());
	return VegetationCalibrationPayloadHashService()
		.attachCanonicalPayloadSha256(unsigned_bundle);
}
