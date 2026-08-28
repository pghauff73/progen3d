#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

VegetationMeasuredSourceArtifact
VegetationMeasuredSourceArtifact::withPayloadSha256(
	std::string payload_sha256) const
{
	return VegetationMeasuredSourceArtifact(
		schema_version_,
		source_identifier_,
		source_citation_,
		source_locator_,
		media_type_,
		coordinate_system_,
		unit_system_,
		acquisition_method_,
		uncertainty_statement_,
		source_payload_,
		std::move(payload_sha256));
}
