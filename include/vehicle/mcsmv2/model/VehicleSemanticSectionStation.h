#pragma once

#include "vehicle/mcsmv2/model/VehicleSectionLandmark.h"
#include "vehicle/mcsmv2/model/VehicleSemanticStationRole.h"

#include <array>
#include <cstddef>
#include <utility>

class VehicleSemanticSectionStation
{
public:
	VehicleSemanticSectionStation(
		std::size_t index,
		VehicleSemanticStationRole role,
		double normalized_station,
		double source_x,
		std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> landmarks,
		double confidence)
		: index_(index),
		  role_(role),
		  normalized_station_(normalized_station),
		  source_x_(source_x),
		  landmarks_(std::move(landmarks)),
		  confidence_(confidence)
	{
	}

	std::size_t index() const { return index_; }
	VehicleSemanticStationRole role() const { return role_; }
	double normalizedStation() const { return normalized_station_; }
	double sourceX() const { return source_x_; }
	const std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> &landmarks() const
	{
		return landmarks_;
	}
	const VehicleSectionLandmark &landmark(VehicleSectionLandmarkRole role) const
	{
		return landmarks_[static_cast<std::size_t>(role)];
	}
	double confidence() const { return confidence_; }

private:
	std::size_t index_ = 0u;
	VehicleSemanticStationRole role_ = VehicleSemanticStationRole::TailFace;
	double normalized_station_ = 0.0;
	double source_x_ = 0.0;
	std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> landmarks_;
	double confidence_ = 0.0;
};
