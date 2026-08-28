#pragma once

#include "vehicle/parametric/model/ParametricBodyDefinition.h"
#include "vehicle/parametric/model/VehicleExteriorFeatureDefinition.h"
#include "vehicle/parametric/model/VehiclePackageParameters.h"
#include "vehicle/parametric/model/VehiclePowertrainIntent.h"
#include "vehicle/parametric/model/VehicleStyleParameters.h"
#include "vehicle/parametric/model/VehicleWheelParameters.h"

#include <string>
#include <utility>

class ModernCarVariantDefinition
{
public:
	ModernCarVariantDefinition(
		std::string identifier,
		std::string display_name,
		std::string description,
		std::string package_basis,
		VehiclePackageParameters package,
		VehicleWheelParameters wheels,
		VehicleStyleParameters style,
		VehiclePowertrainIntent powertrain,
		ParametricBodyDefinition body,
		VehicleExteriorFeatureDefinition exterior_features)
		: identifier_(std::move(identifier)),
		  display_name_(std::move(display_name)),
		  description_(std::move(description)),
		  package_basis_(std::move(package_basis)),
		  package_(std::move(package)),
		  wheels_(std::move(wheels)),
		  style_(std::move(style)),
		  powertrain_(std::move(powertrain)),
		  body_(std::move(body)),
		  exterior_features_(std::move(exterior_features))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &displayName() const { return display_name_; }
	const std::string &description() const { return description_; }
	const std::string &packageBasis() const { return package_basis_; }
	const VehiclePackageParameters &package() const { return package_; }
	const VehicleWheelParameters &wheels() const { return wheels_; }
	const VehicleStyleParameters &style() const { return style_; }
	const VehiclePowertrainIntent &powertrain() const { return powertrain_; }
	const ParametricBodyDefinition &body() const { return body_; }
	const VehicleExteriorFeatureDefinition &exteriorFeatures() const
	{
		return exterior_features_;
	}

private:
	std::string identifier_;
	std::string display_name_;
	std::string description_;
	std::string package_basis_;
	VehiclePackageParameters package_{"", 0, 0, 0, 0, 0, 0, 0};
	VehicleWheelParameters wheels_{0, 0, 0, 0};
	VehicleStyleParameters style_{0, 0, 0, 0, 0, 0, 0, 0, 0, false};
	VehiclePowertrainIntent powertrain_{
		ModernCarPowerSource::Combustion,
		ModernCarDriveIntent::AllWheelDrive,
		false,
		false,
		{},
		0};
	ParametricBodyDefinition body_{{0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0,
		 {"", StationInterpolationKind::Pchip, {}},
		 {"", StationInterpolationKind::Pchip, {}},
		 {"", StationInterpolationKind::Pchip, {}}},
		{0, 0, 0, 0, 0, 0,
		 {"", StationInterpolationKind::Pchip, {}},
		 {"", StationInterpolationKind::Pchip, {}},
		 {"", StationInterpolationKind::Pchip, {}}},
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		WheelhouseDifferenceDefinition(0),
		{0, 0},
		{{}, 0},
		ParametricBodySectionNetwork(0)};
	VehicleExteriorFeatureDefinition exterior_features_{{0, 0, 0}, 0};
};
