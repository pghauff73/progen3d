#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <cmath>
#include <string>

namespace {

VegetationMeasurementUnitNormalizationResult rejected(
	const std::string &canonical_unit,
	const std::string &reason)
{
	return VegetationMeasurementUnitNormalizationResult(
		std::nullopt, canonical_unit, reason);
}

VegetationMeasurementUnitNormalizationResult accepted(
	double value,
	const std::string &canonical_unit)
{
	return VegetationMeasurementUnitNormalizationResult(
		value, canonical_unit, std::string());
}

}

VegetationMeasurementUnitNormalizationResult
VegetationMeasurementUnitNormalizationService::normalize(
	double source_value,
	const std::string &source_unit,
	VegetationMeasurementQuantityKind quantity_kind) const
{
	if (!std::isfinite(source_value)) {
		return rejected(std::string(), "Measured value is not finite.");
	}
	switch (quantity_kind) {
	case VegetationMeasurementQuantityKind::LengthMetres:
		if (source_unit == "m") return accepted(source_value, "m");
		if (source_unit == "cm") return accepted(source_value * 0.01, "m");
		if (source_unit == "mm") return accepted(source_value * 0.001, "m");
		if (source_unit == "um") return accepted(source_value * 1.0e-6, "m");
		if (source_unit == "nm") return accepted(source_value * 1.0e-9, "m");
		return rejected("m", "Length unit is unsupported.");
	case VegetationMeasurementQuantityKind::MassDensityKilogramsPerCubicMetre:
		if (source_unit == "kg/m3") return accepted(source_value, "kg/m3");
		if (source_unit == "g/cm3") {
			return accepted(source_value * 1000.0, "kg/m3");
		}
		return rejected("kg/m3", "Mass-density unit is unsupported.");
	case VegetationMeasurementQuantityKind::PressurePascals:
		if (source_unit == "Pa") return accepted(source_value, "Pa");
		if (source_unit == "kPa") return accepted(source_value * 1.0e3, "Pa");
		if (source_unit == "MPa") return accepted(source_value * 1.0e6, "Pa");
		if (source_unit == "GPa") return accepted(source_value * 1.0e9, "Pa");
		return rejected("Pa", "Pressure unit is unsupported.");
	case VegetationMeasurementQuantityKind::Fraction:
		if (source_unit == "fraction" || source_unit == "dimensionless") {
			return accepted(source_value, "fraction");
		}
		if (source_unit == "percent") {
			return accepted(source_value * 0.01, "fraction");
		}
		return rejected("fraction", "Fraction unit is unsupported.");
	case VegetationMeasurementQuantityKind::AngleDegrees:
		if (source_unit == "deg" || source_unit == "degree" ||
		    source_unit == "degrees") {
			return accepted(source_value, "degree");
		}
		if (source_unit == "rad") {
			return accepted(
				source_value * 180.0 / 3.14159265358979323846, "degree");
		}
		return rejected("degree", "Angle unit is unsupported.");
	case VegetationMeasurementQuantityKind::Dimensionless:
		if (source_unit == "dimensionless") {
			return accepted(source_value, "dimensionless");
		}
		return rejected("dimensionless", "Dimensionless unit is unsupported.");
	}
	return rejected(std::string(), "Measurement quantity is unsupported.");
}
