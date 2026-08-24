#include "vehicle/mcsmv2/service/VehicleFunctionalPackageValidationService.h"

#include <cmath>
#include <set>
#include <string>

namespace {

bool finite(const glm::dvec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
		std::isfinite(value.z);
}

bool containedByPackage(
	const VehiclePackageParameters &package,
	const VehicleFunctionalEnvelope &envelope)
{
	const glm::dvec3 half_extents = envelope.dimensions() * 0.5;
	return envelope.centre().x - half_extents.x >= -package.width() * 0.5 &&
		envelope.centre().x + half_extents.x <= package.width() * 0.5 &&
		envelope.centre().y - half_extents.y >= 0.0 &&
		envelope.centre().y + half_extents.y <= package.height() &&
		envelope.centre().z - half_extents.z >= -package.length() * 0.5 &&
		envelope.centre().z + half_extents.z <= package.length() * 0.5;
}

} // namespace

ModernCarSemanticValidationReport VehicleFunctionalPackageValidationService::validate(
	const ModernCarSemanticVariant &variant,
	const VehicleFunctionalPackageSystem &system) const
{
	ModernCarSemanticValidationReport report;
	std::set<std::string> identifiers;
	for (const VehicleFunctionalEnvelope &envelope : system.envelopes()) {
		if (envelope.identifier().empty() || envelope.purpose().empty() ||
		    !identifiers.insert(envelope.identifier()).second ||
		    !finite(envelope.centre()) || !finite(envelope.dimensions()) ||
		    envelope.dimensions().x <= 0.0 || envelope.dimensions().y <= 0.0 ||
		    envelope.dimensions().z <= 0.0 ||
		    !containedByPackage(variant.package(), envelope)) {
			report.addIssue({
				ModernCarSemanticValidationCode::FunctionalPackageFailure,
				"Functional envelopes must be unique, finite, positive, and contained by the vehicle package.",
				{variant.identifier(), envelope.identifier()}});
		}
	}
	const std::size_t expected_count = variant.powertrain().hasBatteryPack() ? 3u : 2u;
	if (system.envelopes().size() != expected_count) {
		report.addIssue({
			ModernCarSemanticValidationCode::FunctionalPackageFailure,
			"Functional package envelope count does not match the powertrain architecture.",
			{variant.identifier()}});
	}
	return report;
}
