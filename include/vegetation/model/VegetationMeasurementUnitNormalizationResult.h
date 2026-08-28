#pragma once

#include <optional>
#include <string>
#include <utility>

enum class VegetationMeasurementQuantityKind
{
	LengthMetres,
	MassDensityKilogramsPerCubicMetre,
	PressurePascals,
	Fraction,
	AngleDegrees,
	Dimensionless
};

class VegetationMeasurementUnitNormalizationResult
{
public:
	VegetationMeasurementUnitNormalizationResult(
		std::optional<double> normalized_value,
		std::string canonical_unit,
		std::string rejection_reason)
		: normalized_value_(normalized_value),
		  canonical_unit_(std::move(canonical_unit)),
		  rejection_reason_(std::move(rejection_reason))
	{
	}

	bool succeeded() const { return normalized_value_.has_value(); }
	std::optional<double> normalizedValue() const { return normalized_value_; }
	const std::string &canonicalUnit() const { return canonical_unit_; }
	const std::string &rejectionReason() const { return rejection_reason_; }

private:
	std::optional<double> normalized_value_;
	std::string canonical_unit_;
	std::string rejection_reason_;
};
