#include "vehicle/mcsmv2/service/VehicleOccupantEnvelopeConstructionService.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

const GeneratedMcsMv2VariantRecord &sourceRecordFor(
	const std::string &variant_identifier)
{
	for (const GeneratedMcsMv2VariantRecord &record :
	     generatedMcsMv2VariantRecords()) {
		if (record.key == variant_identifier) return record;
	}
	throw std::invalid_argument(
		"No generated MCSMv2 source record exists for variant '" +
		variant_identifier + "'.");
}

bool contains(const std::string &value, const char *fragment)
{
	return value.find(fragment) != std::string::npos;
}

} // namespace

VehicleOccupantEnvelopeSystem VehicleOccupantEnvelopeConstructionService::construct(
	const ModernCarSemanticVariant &variant) const
{
	const GeneratedMcsMv2VariantRecord &record =
		sourceRecordFor(variant.identifier());
	const ModernCarPlatformDefinition &platform = variant.platform();
	const double package_centre = variant.package().sourcePackageCenterStation();
	std::vector<VehicleOccupantEnvelope> envelopes;
	envelopes.reserve(record.occupant_envelope_identifiers.size());
	for (std::string_view accepted_identifier :
	     record.occupant_envelope_identifiers) {
		const std::string identifier(accepted_identifier);
		const bool front_row = contains(identifier, "front");
		const bool left_side = contains(identifier, "left");
		const bool head = contains(identifier, "head");
		const double source_x = front_row
			? platform.frontHpointX()
			: platform.rearHpointX();
		const double source_y = left_side
			? -platform.seatLateralOffset()
			: platform.seatLateralOffset();
		const glm::dvec3 centre(
			source_y,
			head ? platform.eyeHeight() + 0.015 : platform.hpointHeight() + 0.29,
			source_x - package_centre);
		envelopes.emplace_back(
			identifier,
			head ? VehicleOccupantEnvelopeRole::Head
			     : VehicleOccupantEnvelopeRole::Torso,
			centre,
			head ? glm::dvec3(0.115, 0.125, 0.105)
			     : glm::dvec3(0.205, 0.285, 0.165));
	}
	return VehicleOccupantEnvelopeSystem(
		variant.identifier(), std::move(envelopes));
}
