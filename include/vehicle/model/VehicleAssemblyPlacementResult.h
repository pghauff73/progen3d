#pragma once

#include "spatial/model/SpatialResolutionRecord.h"
#include "vehicle/model/ModernVehicleAssembly.h"

#include <optional>
#include <string>
#include <utility>

class VehicleAssemblyPlacement
{
public:
	VehicleAssemblyPlacement(
		VehiclePlacedAssembly placed_assembly,
		SpatialResolutionRecord resolution_record)
		: placed_assembly_(std::move(placed_assembly)),
		  resolution_record_(std::move(resolution_record))
	{
	}

	const VehiclePlacedAssembly &placedAssembly() const
	{
		return placed_assembly_;
	}
	const SpatialResolutionRecord &resolutionRecord() const
	{
		return resolution_record_;
	}

private:
	VehiclePlacedAssembly placed_assembly_;
	SpatialResolutionRecord resolution_record_;
};

class VehicleAssemblyPlacementResult
{
public:
	static VehicleAssemblyPlacementResult succeeded(
		VehicleAssemblyPlacement placement)
	{
		return VehicleAssemblyPlacementResult(std::move(placement), {});
	}

	static VehicleAssemblyPlacementResult failed(std::string diagnostic)
	{
		return VehicleAssemblyPlacementResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return placement_.has_value(); }
	const std::optional<VehicleAssemblyPlacement> &placement() const
	{
		return placement_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VehicleAssemblyPlacementResult(
		std::optional<VehicleAssemblyPlacement> placement,
		std::string diagnostic)
		: placement_(std::move(placement)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VehicleAssemblyPlacement> placement_;
	std::string diagnostic_;
};
