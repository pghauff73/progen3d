#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"
#include "vegetation/model/VegetationCalibrationMeasurement.h"
#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

#include <cstddef>
#include <string>

class VegetationMeasuredCalibrationMeasurementFactory
{
public:
	VegetationCalibrationMeasurement createDecimal(
		VegetationCalibrationMeasurementKind kind,
		double value,
		const std::string &unit,
		const VegetationCalibrationSubjectScope &subject_scope,
		const VegetationMeasuredSourceArtifact &artifact) const;

	VegetationCalibrationMeasurement createCount(
		VegetationCalibrationMeasurementKind kind,
		std::size_t value,
		const VegetationCalibrationSubjectScope &subject_scope,
		const VegetationMeasuredSourceArtifact &artifact) const;

	VegetationCalibrationMeasurement createText(
		VegetationCalibrationMeasurementKind kind,
		std::string value,
		const std::string &unit,
		const VegetationCalibrationSubjectScope &subject_scope,
		const VegetationMeasuredSourceArtifact &artifact) const;
};
