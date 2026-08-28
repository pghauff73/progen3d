#pragma once

#include <string>
#include <utility>

class VegetationCanopyObservationRecord
{
public:
	VegetationCanopyObservationRecord(
		std::string observation_identifier,
		double leaf_area_index,
		double crown_gap_fraction,
		double mean_leaf_inclination_degrees,
		std::string point_cloud_identifier)
		: observation_identifier_(std::move(observation_identifier)),
		  leaf_area_index_(leaf_area_index),
		  crown_gap_fraction_(crown_gap_fraction),
		  mean_leaf_inclination_degrees_(mean_leaf_inclination_degrees),
		  point_cloud_identifier_(std::move(point_cloud_identifier))
	{
	}

	const std::string &observationIdentifier() const
	{
		return observation_identifier_;
	}
	double leafAreaIndex() const { return leaf_area_index_; }
	double crownGapFraction() const { return crown_gap_fraction_; }
	double meanLeafInclinationDegrees() const
	{
		return mean_leaf_inclination_degrees_;
	}
	const std::string &pointCloudIdentifier() const
	{
		return point_cloud_identifier_;
	}

private:
	std::string observation_identifier_;
	double leaf_area_index_ = 0.0;
	double crown_gap_fraction_ = 0.0;
	double mean_leaf_inclination_degrees_ = 0.0;
	std::string point_cloud_identifier_;
};
