#include "vehicle/mcsmv2/service/VehicleOccupantEnvelopeValidationService.h"

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
	const glm::dvec3 &centre,
	const glm::dvec3 &half_extents)
{
	return centre.x - half_extents.x >= -package.width() * 0.5 &&
		centre.x + half_extents.x <= package.width() * 0.5 &&
		centre.y - half_extents.y >= 0.0 &&
		centre.y + half_extents.y <= package.height() &&
		centre.z - half_extents.z >= -package.length() * 0.5 &&
		centre.z + half_extents.z <= package.length() * 0.5;
}

} // namespace

ModernCarSemanticValidationReport VehicleOccupantEnvelopeValidationService::validate(
	const ModernCarSemanticVariant &variant,
	const VehicleOccupantEnvelopeSystem &system) const
{
	ModernCarSemanticValidationReport report;
	std::set<std::string> identifiers;
	std::size_t head_count = 0u;
	std::size_t torso_count = 0u;
	for (const VehicleOccupantEnvelope &envelope : system.envelopes()) {
		if (envelope.role() == VehicleOccupantEnvelopeRole::Head) ++head_count;
		else ++torso_count;
		if (envelope.identifier().empty() ||
		    !identifiers.insert(envelope.identifier()).second ||
		    !finite(envelope.centre()) || !finite(envelope.halfExtents()) ||
		    envelope.halfExtents().x <= 0.0 || envelope.halfExtents().y <= 0.0 ||
		    envelope.halfExtents().z <= 0.0 ||
		    !containedByPackage(
			    variant.package(), envelope.centre(), envelope.halfExtents())) {
			report.addIssue({
				ModernCarSemanticValidationCode::OccupantContainmentFailure,
				"Occupant envelopes must be unique, finite, positive, and contained by the vehicle package.",
				{variant.identifier(), envelope.identifier()}});
		}
	}
	if (system.envelopes().size() != 8u || head_count != 4u || torso_count != 4u) {
		report.addIssue({
			ModernCarSemanticValidationCode::OccupantContainmentFailure,
			"MCSMv2 requires four head and four torso occupant envelopes.",
			{variant.identifier()}});
	}
	return report;
}
