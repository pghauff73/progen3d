#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantHydraulicProfile
{
public:
	PlantHydraulicProfile(
		std::optional<double>
			maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal,
		std::optional<double> hydraulic_capacitance_kilograms_per_megapascal,
		std::optional<double>
			xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals,
		std::optional<double>
			stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals,
		std::optional<double> leaf_to_sapwood_area_ratio,
		std::vector<std::string> evidence_identifiers = {})
		: maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal_(
			  maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal),
		  hydraulic_capacitance_kilograms_per_megapascal_(
			  hydraulic_capacitance_kilograms_per_megapascal),
		  xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals_(
			  xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals),
		  stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals_(
			  stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals),
		  leaf_to_sapwood_area_ratio_(leaf_to_sapwood_area_ratio),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantHydraulicProfile uncalibrated()
	{
		return PlantHydraulicProfile(
			std::nullopt,
			std::nullopt,
			std::nullopt,
			std::nullopt,
			std::nullopt);
	}

	std::optional<double>
	maximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal()
		const
	{
		return maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal_;
	}
	std::optional<double> hydraulicCapacitanceKilogramsPerMegapascal() const
	{
		return hydraulic_capacitance_kilograms_per_megapascal_;
	}
	std::optional<double>
	xylemWaterPotentialAtFiftyPercentConductivityLossMegapascals() const
	{
		return xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals_;
	}
	std::optional<double>
	stomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals() const
	{
		return stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals_;
	}
	std::optional<double> leafToSapwoodAreaRatio() const
	{
		return leaf_to_sapwood_area_ratio_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedWaterTransport() const
	{
		return maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal_.has_value() &&
		       maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal_.value() > 0.0 &&
		       hydraulic_capacitance_kilograms_per_megapascal_.has_value() &&
		       hydraulic_capacitance_kilograms_per_megapascal_.value() > 0.0 &&
		       xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals_.has_value() &&
		       xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals_.value() < 0.0 &&
		       stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals_.has_value() &&
		       stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals_.value() < 0.0 &&
		       leaf_to_sapwood_area_ratio_.has_value() &&
		       leaf_to_sapwood_area_ratio_.value() > 0.0 &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double>
		maximum_leaf_specific_conductance_millimoles_per_square_metre_per_second_per_megapascal_;
	std::optional<double> hydraulic_capacitance_kilograms_per_megapascal_;
	std::optional<double>
		xylem_water_potential_at_fifty_percent_conductivity_loss_megapascals_;
	std::optional<double>
		stomatal_water_potential_at_fifty_percent_conductance_loss_megapascals_;
	std::optional<double> leaf_to_sapwood_area_ratio_;
	std::vector<std::string> evidence_identifiers_;
};
