#pragma once

#include "vehicle/model/VehiclePackage.h"

#include <string>
#include <utility>

enum class ModernCarPowerSource
{
	Combustion,
	BatteryElectric
};

enum class ModernCarDriveIntent
{
	AllWheelDrive
};

class VehiclePowertrainIntent
{
public:
	VehiclePowertrainIntent(
		ModernCarPowerSource power_source,
		ModernCarDriveIntent drive_intent,
		bool drives_front_axle,
		bool drives_rear_axle,
		VehiclePackageEnvelope energy_storage_envelope,
		int exhaust_outlet_count)
		: power_source_(power_source),
		  drive_intent_(drive_intent),
		  drives_front_axle_(drives_front_axle),
		  drives_rear_axle_(drives_rear_axle),
		  energy_storage_envelope_(std::move(energy_storage_envelope)),
		  exhaust_outlet_count_(exhaust_outlet_count)
	{
	}

	ModernCarPowerSource powerSource() const { return power_source_; }
	ModernCarDriveIntent driveIntent() const { return drive_intent_; }
	bool drivesFrontAxle() const { return drives_front_axle_; }
	bool drivesRearAxle() const { return drives_rear_axle_; }
	const VehiclePackageEnvelope &energyStorageEnvelope() const
	{
		return energy_storage_envelope_;
	}
	int exhaustOutletCount() const { return exhaust_outlet_count_; }

private:
	ModernCarPowerSource power_source_ = ModernCarPowerSource::Combustion;
	ModernCarDriveIntent drive_intent_ = ModernCarDriveIntent::AllWheelDrive;
	bool drives_front_axle_ = false;
	bool drives_rear_axle_ = false;
	VehiclePackageEnvelope energy_storage_envelope_;
	int exhaust_outlet_count_ = 0;
};
