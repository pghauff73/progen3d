#pragma once

#include "vehicle/model/VehicleIntent.h"
#include "vehicle/model/VehiclePackage.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class VehicleDriveUnitSpecification
{
public:
	VehicleDriveUnitSpecification(
		std::string identifier,
		glm::vec3 origin,
		bool drives_front_axle,
		bool drives_rear_axle)
		: identifier_(std::move(identifier)),
		  origin_(origin),
		  drives_front_axle_(drives_front_axle),
		  drives_rear_axle_(drives_rear_axle)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &origin() const { return origin_; }
	bool drivesFrontAxle() const { return drives_front_axle_; }
	bool drivesRearAxle() const { return drives_rear_axle_; }

private:
	std::string identifier_;
	glm::vec3 origin_{0.0f};
	bool drives_front_axle_ = false;
	bool drives_rear_axle_ = false;
};

class VehiclePowertrainSpecification
{
public:
	VehiclePowertrainSpecification(
		VehiclePowertrainType powertrain_type,
		VehicleDriveLayout drive_layout,
		VehiclePackageEnvelope energy_storage_envelope,
		std::vector<VehicleDriveUnitSpecification> drive_units,
		glm::vec3 underbody_dimensions,
		glm::vec3 underbody_origin)
		: powertrain_type_(powertrain_type),
		  drive_layout_(drive_layout),
		  energy_storage_envelope_(std::move(energy_storage_envelope)),
		  drive_units_(std::move(drive_units)),
		  underbody_dimensions_(underbody_dimensions),
		  underbody_origin_(underbody_origin)
	{
	}

	VehiclePowertrainType powertrainType() const { return powertrain_type_; }
	VehicleDriveLayout driveLayout() const { return drive_layout_; }
	const VehiclePackageEnvelope &energyStorageEnvelope() const
	{
		return energy_storage_envelope_;
	}
	const std::vector<VehicleDriveUnitSpecification> &driveUnits() const
	{
		return drive_units_;
	}
	const glm::vec3 &underbodyDimensions() const
	{
		return underbody_dimensions_;
	}
	const glm::vec3 &underbodyOrigin() const { return underbody_origin_; }

private:
	VehiclePowertrainType powertrain_type_ =
		VehiclePowertrainType::BatteryElectric;
	VehicleDriveLayout drive_layout_ = VehicleDriveLayout::AllWheelDrive;
	VehiclePackageEnvelope energy_storage_envelope_;
	std::vector<VehicleDriveUnitSpecification> drive_units_;
	glm::vec3 underbody_dimensions_{0.0f};
	glm::vec3 underbody_origin_{0.0f};
};
