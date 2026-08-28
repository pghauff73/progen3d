#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantBiomechanicalProfile
{
public:
	PlantBiomechanicalProfile(
		std::optional<double> mass_density_kilograms_per_cubic_metre,
		std::optional<double> elastic_modulus_pascals,
		std::optional<double> damping_ratio,
		std::optional<double> drag_coefficient,
		std::vector<std::string> evidence_identifiers = {})
		: mass_density_kilograms_per_cubic_metre_(
			  mass_density_kilograms_per_cubic_metre),
		  elastic_modulus_pascals_(elastic_modulus_pascals),
		  damping_ratio_(damping_ratio),
		  drag_coefficient_(drag_coefficient),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantBiomechanicalProfile uncalibrated()
	{
		return PlantBiomechanicalProfile(
			std::nullopt, std::nullopt, std::nullopt, std::nullopt);
	}

	std::optional<double> massDensityKilogramsPerCubicMetre() const
	{
		return mass_density_kilograms_per_cubic_metre_;
	}
	std::optional<double> elasticModulusPascals() const
	{
		return elastic_modulus_pascals_;
	}
	std::optional<double> dampingRatio() const { return damping_ratio_; }
	std::optional<double> dragCoefficient() const { return drag_coefficient_; }
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedWindSimulation() const
	{
		return mass_density_kilograms_per_cubic_metre_.has_value() &&
		       mass_density_kilograms_per_cubic_metre_.value() > 0.0 &&
		       elastic_modulus_pascals_.has_value() &&
		       elastic_modulus_pascals_.value() > 0.0 &&
		       damping_ratio_.has_value() && damping_ratio_.value() >= 0.0 &&
		       drag_coefficient_.has_value() && drag_coefficient_.value() > 0.0 &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double> mass_density_kilograms_per_cubic_metre_;
	std::optional<double> elastic_modulus_pascals_;
	std::optional<double> damping_ratio_;
	std::optional<double> drag_coefficient_;
	std::vector<std::string> evidence_identifiers_;
};
