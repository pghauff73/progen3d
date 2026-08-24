#pragma once

#include "vehicle/model/VehicleDatum.h"
#include "vehicle/model/VehicleIntent.h"
#include "vehicle/model/VehiclePackage.h"
#include "vehicle/model/VehicleStyleState.h"

#include <string>
#include <utility>

class VehicleDefinition
{
public:
	VehicleDefinition(
		std::string identifier,
		VehicleIntent intent,
		VehiclePackage package,
		VehicleDatumSet datums,
		VehicleStyleState style)
		: identifier_(std::move(identifier)),
		  intent_(std::move(intent)),
		  package_(std::move(package)),
		  datums_(std::move(datums)),
		  style_(std::move(style))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleIntent &intent() const { return intent_; }
	const VehiclePackage &package() const { return package_; }
	const VehicleDatumSet &datums() const { return datums_; }
	const VehicleStyleState &style() const { return style_; }

private:
	std::string identifier_;
	VehicleIntent intent_;
	VehiclePackage package_;
	VehicleDatumSet datums_;
	VehicleStyleState style_;
};
