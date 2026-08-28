#pragma once

#include "vehicle/mcsmv2/model/VehicleSurfaceCoordinate.h"

#include <stdexcept>
#include <utility>
#include <vector>

class VehicleSurfaceDomainLoop
{
public:
	explicit VehicleSurfaceDomainLoop(std::vector<VehicleSurfaceCoordinate> coordinates)
		: coordinates_(std::move(coordinates))
	{
		if (coordinates_.size() < 4u ||
		    coordinates_.front().longitudinalParameter() !=
			coordinates_.back().longitudinalParameter() ||
		    coordinates_.front().periodicParameter() !=
			coordinates_.back().periodicParameter()) {
			throw std::invalid_argument(
				"Vehicle surface domain loops must contain a closed polygon.");
		}
	}

	const std::vector<VehicleSurfaceCoordinate> &coordinates() const
	{
		return coordinates_;
	}

private:
	std::vector<VehicleSurfaceCoordinate> coordinates_;
};
