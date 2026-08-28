#pragma once

#include <string>
#include <utility>

class VegetationBiomechanicalMaterialTestRecord
{
public:
	VegetationBiomechanicalMaterialTestRecord(
		std::string test_identifier,
		double mass_density_kilograms_per_cubic_metre,
		double elastic_modulus_pascals,
		double damping_ratio,
		double drag_coefficient)
		: test_identifier_(std::move(test_identifier)),
		  mass_density_kilograms_per_cubic_metre_(
			  mass_density_kilograms_per_cubic_metre),
		  elastic_modulus_pascals_(elastic_modulus_pascals),
		  damping_ratio_(damping_ratio),
		  drag_coefficient_(drag_coefficient)
	{
	}

	const std::string &testIdentifier() const { return test_identifier_; }
	double massDensityKilogramsPerCubicMetre() const
	{
		return mass_density_kilograms_per_cubic_metre_;
	}
	double elasticModulusPascals() const { return elastic_modulus_pascals_; }
	double dampingRatio() const { return damping_ratio_; }
	double dragCoefficient() const { return drag_coefficient_; }

private:
	std::string test_identifier_;
	double mass_density_kilograms_per_cubic_metre_ = 0.0;
	double elastic_modulus_pascals_ = 0.0;
	double damping_ratio_ = 0.0;
	double drag_coefficient_ = 0.0;
};
