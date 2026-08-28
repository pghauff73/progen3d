#include "vehicle/service/VehiclePackageValidationService.h"

#include <glm/common.hpp>

#include <cmath>

namespace {

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

bool envelope_inside_package(
	const VehiclePackageEnvelope &envelope,
	const VehiclePackage &package)
{
	if (envelope.identifier().empty()) return true;
	if (!envelope.isFiniteAndOrdered()) return false;
	const float half_width = package.overallWidth() * 0.5f;
	return envelope.minimum().x >= -half_width &&
	       envelope.maximum().x <= half_width &&
	       envelope.minimum().y >= 0.0f &&
	       envelope.maximum().y <= package.overallHeight() &&
	       envelope.minimum().z >= package.rearBumperStation() &&
	       envelope.maximum().z <= package.frontBumperStation();
}

} // namespace

VehicleValidationReport VehiclePackageValidationService::validate(
	const VehicleIntent &intent,
	const VehiclePackage &package) const
{
	VehicleValidationReport report;
	const bool positive_dimensions =
		finite_positive(package.overallLength()) &&
		finite_positive(package.overallWidth()) &&
		finite_positive(package.overallHeight()) &&
		finite_positive(package.wheelbase()) &&
		finite_positive(package.frontTrack()) &&
		finite_positive(package.rearTrack()) &&
		finite_positive(package.groundClearance()) &&
		finite_positive(package.frontOverhang()) &&
		finite_positive(package.rearOverhang()) &&
		finite_positive(package.frontWheelRadius()) &&
		finite_positive(package.rearWheelRadius());
	const float derived_length = package.frontOverhang() + package.wheelbase() +
	                             package.rearOverhang();
	if (!positive_dimensions ||
	    std::fabs(derived_length - package.overallLength()) > 1.0e-4f ||
	    package.wheelbase() >= package.overallLength() ||
	    intent.doorCount() < 2 || intent.doorCount() > 5 ||
	    intent.seatCount() < 1 || intent.seatCount() > 9) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::InvalidWheelbase,
			"Vehicle package dimensions must be finite and satisfy length = front overhang + wheelbase + rear overhang."));
	}
	const float maximum_front_tire_x =
		package.frontTrack() * 0.5f + package.frontWheelRadius() * 0.36f;
	const float maximum_rear_tire_x =
		package.rearTrack() * 0.5f + package.rearWheelRadius() * 0.36f;
	if (maximum_front_tire_x > package.overallWidth() * 0.5f ||
	    maximum_rear_tire_x > package.overallWidth() * 0.5f ||
	    package.groundClearance() >=
			std::min(package.frontWheelRadius(), package.rearWheelRadius())) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::TireOutsideBody,
			"Wheel tracks, tire envelope, and ground clearance must remain inside the vehicle package."));
	}
	for (const VehiclePackageEnvelope *envelope :
	     {&package.cabin(), &package.frontPowertrain(),
	      &package.rearPowertrain(), &package.battery(), &package.luggage()}) {
		if (!envelope_inside_package(*envelope, package)) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::PositioningFailure,
				"Package envelope '" + envelope->identifier() +
					"' is invalid or lies outside the vehicle package."));
		}
	}
	return report;
}
