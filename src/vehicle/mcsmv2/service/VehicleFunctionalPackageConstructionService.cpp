#include "vehicle/mcsmv2/service/VehicleFunctionalPackageConstructionService.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h"

#include <algorithm>
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

VehicleFunctionalEnvelope createEnvelope(
	const ModernCarSemanticVariant &variant,
	const std::string &identifier)
{
	const VehiclePackageParameters &package = variant.package();
	const ModernCarPowertrainDefinition &powertrain = variant.powertrain();
	if (identifier == "engine_envelope") {
		return VehicleFunctionalEnvelope(
			identifier,
			"internal_combustion_engine",
			glm::dvec3(
				0.0,
				package.groundClearance() + powertrain.engineEnvelopeHeight() * 0.5,
				package.frontAxleStation() + package.frontOverhang() * 0.18),
			glm::dvec3(
				powertrain.engineEnvelopeWidth(),
				powertrain.engineEnvelopeHeight(),
				powertrain.engineEnvelopeLength()));
	}
	if (identifier == "rear_differential_envelope") {
		return VehicleFunctionalEnvelope(
			identifier,
			"rear_differential",
			glm::dvec3(0.0, package.groundClearance() + 0.18,
			           package.rearAxleStation()),
			glm::dvec3(0.62, 0.28, 0.42));
	}
	if (identifier == "battery_envelope") {
		return VehicleFunctionalEnvelope(
			identifier,
			"traction_battery",
			glm::dvec3(
				0.0,
				package.groundClearance() + powertrain.batteryThickness() * 0.5,
				(package.frontAxleStation() + package.rearAxleStation()) * 0.5),
			glm::dvec3(
				package.width() * 0.76,
				powertrain.batteryThickness(),
				package.wheelbase() * 0.72));
	}
	if (identifier == "front_eaxle_envelope" ||
	    identifier == "rear_eaxle_envelope") {
		const bool front = identifier == "front_eaxle_envelope";
		const double track = front
			? variant.wheelMotion().wheelGeometry().frontTrack()
			: variant.wheelMotion().wheelGeometry().rearTrack();
		return VehicleFunctionalEnvelope(
			identifier,
			front ? "front_electric_axle" : "rear_electric_axle",
			glm::dvec3(
				0.0,
				package.groundClearance() + 0.17,
				front ? package.frontAxleStation() : package.rearAxleStation()),
			glm::dvec3(std::min(track * 0.72, package.width() * 0.78), 0.28, 0.34));
	}
	throw std::invalid_argument(
		"Unknown MCSMv2 functional envelope identifier '" + identifier + "'.");
}

} // namespace

VehicleFunctionalPackageSystem VehicleFunctionalPackageConstructionService::construct(
	const ModernCarSemanticVariant &variant) const
{
	const GeneratedMcsMv2VariantRecord &record =
		sourceRecordFor(variant.identifier());
	std::vector<VehicleFunctionalEnvelope> envelopes;
	envelopes.reserve(record.functional_envelope_count);
	for (std::size_t index = 0u; index < record.functional_envelope_count; ++index) {
		envelopes.push_back(createEnvelope(
			variant, std::string(record.functional_envelope_identifiers[index])));
	}
	return VehicleFunctionalPackageSystem(
		variant.identifier(), std::move(envelopes));
}
