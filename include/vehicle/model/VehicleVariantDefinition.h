#pragma once

#include "vehicle/model/VehicleMvp25Reference.h"
#include "vehicle/model/VehiclePowertrainSpecification.h"

#include <string>
#include <utility>

class VehicleVariantDefinition
{
public:
	VehicleVariantDefinition(
		std::string identifier,
		std::string display_name,
		VehiclePackageEvidence package_candidate,
		VehiclePowertrainSpecification powertrain_candidate)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  package_candidate_(std::move(package_candidate)),
		  powertrain_candidate_(std::move(powertrain_candidate))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const VehiclePackageEvidence &packageCandidate() const
	{
		return package_candidate_;
	}
	const VehiclePowertrainSpecification &powertrainCandidate() const
	{
		return powertrain_candidate_;
	}

private:
	std::string identifier_;
	std::string display_name_;
	VehiclePackageEvidence package_candidate_{
		0, 0, 0, 0, 0, 0, 0, 0, {}, {},
		VehicleDriveEvidence::AllWheelDrive, 0};
	VehiclePowertrainSpecification powertrain_candidate_{
		VehiclePowertrainType::BatteryElectric,
		VehicleDriveLayout::AllWheelDrive,
		{}, {}, {}, {}};
};
