#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantSizeAllometryProfile
{
public:
	PlantSizeAllometryProfile(
		std::optional<double> reference_height_metres,
		std::optional<double> reference_horizontal_radius_metres,
		std::optional<double> reference_supporting_axis_diameter_metres,
		std::string size_relationship_model_identifier,
		std::vector<std::string> evidence_identifiers = {})
		: reference_height_metres_(reference_height_metres),
		  reference_horizontal_radius_metres_(
			  reference_horizontal_radius_metres),
		  reference_supporting_axis_diameter_metres_(
			  reference_supporting_axis_diameter_metres),
		  size_relationship_model_identifier_(
			  std::move(size_relationship_model_identifier)),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantSizeAllometryProfile uncalibrated()
	{
		return PlantSizeAllometryProfile(
			std::nullopt, std::nullopt, std::nullopt, std::string());
	}

	std::optional<double> referenceHeightMetres() const
	{
		return reference_height_metres_;
	}
	std::optional<double> referenceHorizontalRadiusMetres() const
	{
		return reference_horizontal_radius_metres_;
	}
	std::optional<double> referenceSupportingAxisDiameterMetres() const
	{
		return reference_supporting_axis_diameter_metres_;
	}
	const std::string &sizeRelationshipModelIdentifier() const
	{
		return size_relationship_model_identifier_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedSizeProjection() const
	{
		return reference_height_metres_.has_value() &&
		       reference_height_metres_.value() > 0.0 &&
		       reference_horizontal_radius_metres_.has_value() &&
		       reference_horizontal_radius_metres_.value() > 0.0 &&
		       !size_relationship_model_identifier_.empty() &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double> reference_height_metres_;
	std::optional<double> reference_horizontal_radius_metres_;
	std::optional<double> reference_supporting_axis_diameter_metres_;
	std::string size_relationship_model_identifier_;
	std::vector<std::string> evidence_identifiers_;
};
