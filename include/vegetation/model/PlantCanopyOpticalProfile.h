#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantCanopyOpticalProfile
{
public:
	PlantCanopyOpticalProfile(
		std::optional<double> leaf_area_index,
		std::optional<double> crown_gap_fraction,
		std::optional<double> mean_leaf_inclination_degrees,
		std::vector<std::string> evidence_identifiers = {})
		: leaf_area_index_(leaf_area_index),
		  crown_gap_fraction_(crown_gap_fraction),
		  mean_leaf_inclination_degrees_(mean_leaf_inclination_degrees),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantCanopyOpticalProfile uncalibrated()
	{
		return PlantCanopyOpticalProfile(
			std::nullopt, std::nullopt, std::nullopt);
	}

	std::optional<double> leafAreaIndex() const { return leaf_area_index_; }
	std::optional<double> crownGapFraction() const
	{
		return crown_gap_fraction_;
	}
	std::optional<double> meanLeafInclinationDegrees() const
	{
		return mean_leaf_inclination_degrees_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool supportsCalibratedLightInterception() const
	{
		return leaf_area_index_.has_value() && leaf_area_index_.value() >= 0.0 &&
		       crown_gap_fraction_.has_value() &&
		       crown_gap_fraction_.value() >= 0.0 &&
		       crown_gap_fraction_.value() <= 1.0 &&
		       mean_leaf_inclination_degrees_.has_value() &&
		       mean_leaf_inclination_degrees_.value() >= 0.0 &&
		       mean_leaf_inclination_degrees_.value() <= 90.0 &&
		       !evidence_identifiers_.empty();
	}

private:
	std::optional<double> leaf_area_index_;
	std::optional<double> crown_gap_fraction_;
	std::optional<double> mean_leaf_inclination_degrees_;
	std::vector<std::string> evidence_identifiers_;
};
