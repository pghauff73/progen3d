#pragma once

#include "vehicle/mcsmv2/model/VehicleSemanticSectionStation.h"

#include <string>
#include <utility>
#include <vector>

class VehicleSemanticSectionField
{
public:
	VehicleSemanticSectionField(
		std::string identifier,
		std::vector<VehicleSemanticSectionStation> stations)
		: identifier_(std::move(identifier)),
		  stations_(std::move(stations))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<VehicleSemanticSectionStation> &stations() const
	{
		return stations_;
	}
	double rearSourceX() const { return stations_.front().sourceX(); }
	double frontSourceX() const { return stations_.back().sourceX(); }

private:
	std::string identifier_;
	std::vector<VehicleSemanticSectionStation> stations_;
};
