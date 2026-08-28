#include "vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.h"

#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<std::string> evidence_identifiers(
	const VegetationMeasuredSourceArtifact &artifact)
{
	return {artifact.sourceIdentifier()};
}

std::string domain_name(VegetationCalibrationMeasurementKind kind)
{
	return vegetationCalibrationDomainName(
		vegetationCalibrationMeasurementDomain(kind));
}

}

VegetationCalibrationMeasurement
VegetationMeasuredCalibrationMeasurementFactory::createDecimal(
	VegetationCalibrationMeasurementKind kind,
	double value,
	const std::string &unit,
	const VegetationCalibrationSubjectScope &subject_scope,
	const VegetationMeasuredSourceArtifact &artifact) const
{
	return VegetationCalibrationMeasurement(
		kind,
		domain_name(kind),
		VegetationCalibrationMeasurementValue(value),
		unit,
		subject_scope.subjectIdentifier(),
		artifact.uncertaintyStatement(),
		evidence_identifiers(artifact));
}

VegetationCalibrationMeasurement
VegetationMeasuredCalibrationMeasurementFactory::createCount(
	VegetationCalibrationMeasurementKind kind,
	std::size_t value,
	const VegetationCalibrationSubjectScope &subject_scope,
	const VegetationMeasuredSourceArtifact &artifact) const
{
	return VegetationCalibrationMeasurement(
		kind,
		domain_name(kind),
		VegetationCalibrationMeasurementValue(value),
		"count",
		subject_scope.subjectIdentifier(),
		artifact.uncertaintyStatement(),
		evidence_identifiers(artifact));
}

VegetationCalibrationMeasurement
VegetationMeasuredCalibrationMeasurementFactory::createText(
	VegetationCalibrationMeasurementKind kind,
	std::string value,
	const std::string &unit,
	const VegetationCalibrationSubjectScope &subject_scope,
	const VegetationMeasuredSourceArtifact &artifact) const
{
	return VegetationCalibrationMeasurement(
		kind,
		domain_name(kind),
		VegetationCalibrationMeasurementValue(std::move(value)),
		unit,
		subject_scope.subjectIdentifier(),
		artifact.uncertaintyStatement(),
		evidence_identifiers(artifact));
}
