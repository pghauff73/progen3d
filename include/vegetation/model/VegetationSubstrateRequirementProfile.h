#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class VegetationSubstrateRequirementProfile
{
public:
	VegetationSubstrateRequirementProfile(
		std::optional<double> minimum_rootable_volume_cubic_metres,
		std::optional<double> minimum_rootable_depth_metres,
		std::optional<double> maximum_bulk_density_kilograms_per_cubic_metre,
		std::optional<double> minimum_air_filled_porosity_fraction,
		std::optional<double> minimum_available_water_capacity_fraction,
		std::vector<std::string> evidence_identifiers = {})
		: minimum_rootable_volume_cubic_metres_(
			  minimum_rootable_volume_cubic_metres),
		  minimum_rootable_depth_metres_(minimum_rootable_depth_metres),
		  maximum_bulk_density_kilograms_per_cubic_metre_(
			  maximum_bulk_density_kilograms_per_cubic_metre),
		  minimum_air_filled_porosity_fraction_(
			  minimum_air_filled_porosity_fraction),
		  minimum_available_water_capacity_fraction_(
			  minimum_available_water_capacity_fraction),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static VegetationSubstrateRequirementProfile uncalibrated()
	{
		return VegetationSubstrateRequirementProfile(
			std::nullopt,
			std::nullopt,
			std::nullopt,
			std::nullopt,
			std::nullopt);
	}

	std::optional<double> minimumRootableVolumeCubicMetres() const
	{
		return minimum_rootable_volume_cubic_metres_;
	}
	std::optional<double> minimumRootableDepthMetres() const
	{
		return minimum_rootable_depth_metres_;
	}
	std::optional<double> maximumBulkDensityKilogramsPerCubicMetre() const
	{
		return maximum_bulk_density_kilograms_per_cubic_metre_;
	}
	std::optional<double> minimumAirFilledPorosityFraction() const
	{
		return minimum_air_filled_porosity_fraction_;
	}
	std::optional<double> minimumAvailableWaterCapacityFraction() const
	{
		return minimum_available_water_capacity_fraction_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedSubstrateSuitability() const
	{
		return minimum_rootable_volume_cubic_metres_.has_value() &&
		       minimum_rootable_volume_cubic_metres_.value() > 0.0 &&
		       minimum_rootable_depth_metres_.has_value() &&
		       minimum_rootable_depth_metres_.value() > 0.0 &&
		       maximum_bulk_density_kilograms_per_cubic_metre_.has_value() &&
		       maximum_bulk_density_kilograms_per_cubic_metre_.value() > 0.0 &&
		       minimum_air_filled_porosity_fraction_.has_value() &&
		       minimum_air_filled_porosity_fraction_.value() > 0.0 &&
		       minimum_air_filled_porosity_fraction_.value() <= 1.0 &&
		       minimum_available_water_capacity_fraction_.has_value() &&
		       minimum_available_water_capacity_fraction_.value() > 0.0 &&
		       minimum_available_water_capacity_fraction_.value() <= 1.0 &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double> minimum_rootable_volume_cubic_metres_;
	std::optional<double> minimum_rootable_depth_metres_;
	std::optional<double> maximum_bulk_density_kilograms_per_cubic_metre_;
	std::optional<double> minimum_air_filled_porosity_fraction_;
	std::optional<double> minimum_available_water_capacity_fraction_;
	std::vector<std::string> evidence_identifiers_;
};
