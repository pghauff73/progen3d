#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantFunctionalTraitProfile
{
public:
	PlantFunctionalTraitProfile(
		std::optional<double> specific_leaf_area_square_metres_per_kilogram,
		std::optional<double> leaf_dry_matter_content_kilograms_per_kilogram,
		std::optional<double> leaf_nitrogen_content_kilograms_per_kilogram,
		std::optional<double> maximum_mature_height_metres,
		std::vector<std::string> evidence_identifiers = {})
		: specific_leaf_area_square_metres_per_kilogram_(
			  specific_leaf_area_square_metres_per_kilogram),
		  leaf_dry_matter_content_kilograms_per_kilogram_(
			  leaf_dry_matter_content_kilograms_per_kilogram),
		  leaf_nitrogen_content_kilograms_per_kilogram_(
			  leaf_nitrogen_content_kilograms_per_kilogram),
		  maximum_mature_height_metres_(maximum_mature_height_metres),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantFunctionalTraitProfile uncalibrated()
	{
		return PlantFunctionalTraitProfile(
			std::nullopt, std::nullopt, std::nullopt, std::nullopt);
	}

	std::optional<double> specificLeafAreaSquareMetresPerKilogram() const
	{
		return specific_leaf_area_square_metres_per_kilogram_;
	}
	std::optional<double> leafDryMatterContentKilogramsPerKilogram() const
	{
		return leaf_dry_matter_content_kilograms_per_kilogram_;
	}
	std::optional<double> leafNitrogenContentKilogramsPerKilogram() const
	{
		return leaf_nitrogen_content_kilograms_per_kilogram_;
	}
	std::optional<double> maximumMatureHeightMetres() const
	{
		return maximum_mature_height_metres_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedFunctionalTraitInference() const
	{
		return specific_leaf_area_square_metres_per_kilogram_.has_value() &&
		       specific_leaf_area_square_metres_per_kilogram_.value() > 0.0 &&
		       leaf_dry_matter_content_kilograms_per_kilogram_.has_value() &&
		       leaf_dry_matter_content_kilograms_per_kilogram_.value() > 0.0 &&
		       leaf_dry_matter_content_kilograms_per_kilogram_.value() <= 1.0 &&
		       leaf_nitrogen_content_kilograms_per_kilogram_.has_value() &&
		       leaf_nitrogen_content_kilograms_per_kilogram_.value() > 0.0 &&
		       leaf_nitrogen_content_kilograms_per_kilogram_.value() <= 1.0 &&
		       maximum_mature_height_metres_.has_value() &&
		       maximum_mature_height_metres_.value() > 0.0 &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double> specific_leaf_area_square_metres_per_kilogram_;
	std::optional<double> leaf_dry_matter_content_kilograms_per_kilogram_;
	std::optional<double> leaf_nitrogen_content_kilograms_per_kilogram_;
	std::optional<double> maximum_mature_height_metres_;
	std::vector<std::string> evidence_identifiers_;
};
