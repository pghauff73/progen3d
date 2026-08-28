#pragma once

#include "vehicle/mcsmv2/model/VehicleCharacterCurve.h"

#include <string>
#include <utility>
#include <vector>

class VehicleCharacterCurveNetwork
{
public:
	VehicleCharacterCurveNetwork(
		std::string identifier,
		std::vector<VehicleCharacterCurve> curves)
		: identifier_(std::move(identifier)),
		  curves_(std::move(curves))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<VehicleCharacterCurve> &curves() const { return curves_; }

private:
	std::string identifier_;
	std::vector<VehicleCharacterCurve> curves_;
};
