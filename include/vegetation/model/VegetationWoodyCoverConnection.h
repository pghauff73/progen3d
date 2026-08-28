#pragma once

#include <string>
#include <utility>

class VegetationWoodyCoverConnection
{
public:
	VegetationWoodyCoverConnection(
		std::string parent_cover_identifier,
		std::string child_cover_identifier,
		double centre_distance_metres)
		: parent_cover_identifier_(std::move(parent_cover_identifier)),
		  child_cover_identifier_(std::move(child_cover_identifier)),
		  centre_distance_metres_(centre_distance_metres)
	{
	}

	const std::string &parentCoverIdentifier() const
	{
		return parent_cover_identifier_;
	}
	const std::string &childCoverIdentifier() const
	{
		return child_cover_identifier_;
	}
	double centreDistanceMetres() const { return centre_distance_metres_; }

private:
	std::string parent_cover_identifier_;
	std::string child_cover_identifier_;
	double centre_distance_metres_ = 0.0;
};
