#include "vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.h"

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"

#include <json/json.h>

#include <string>

VegetationMeasuredSourceArtifact
VegetationDecodedMeasuredSourceArtifactFactory::create(
	const VegetationMeasuredSourceArtifact &source_artifact,
	const VegetationMeasuredSourceDecodeContext &context,
	const std::string &decoder_identifier,
	const std::string &canonical_media_type,
	Json::Value canonical_document) const
{
	Json::Value provenance(Json::objectValue);
	provenance["source_identifier"] = source_artifact.sourceIdentifier();
	provenance["source_media_type"] = source_artifact.mediaType();
	provenance["source_schema_version"] = context.sourceSchemaVersion();
	provenance["source_payload_sha256"] = source_artifact.payloadSha256();
	provenance["decoder_identifier"] = decoder_identifier;
	if (context.coordinateReferenceTransform().has_value()) {
		const auto &transform = *context.coordinateReferenceTransform();
		provenance["coordinate_transform_identifier"] =
			transform.transformIdentifier();
		provenance["coordinate_transform_evidence_identifier"] =
			transform.evidenceIdentifier();
		provenance["coordinate_transform_evidence_sha256"] =
			transform.evidenceSha256();
	}
	canonical_document["source_provenance"] = std::move(provenance);

	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	writer["precision"] = 17;
	writer["precisionType"] = "significant";
	const std::string payload = Json::writeString(writer, canonical_document);
	const std::string canonical_source_identifier =
		source_artifact.sourceIdentifier() + ":canonical:" + decoder_identifier;
	const VegetationMeasuredSourceArtifact unsigned_artifact(
		"ProGen3D-VegetationMeasuredSourceArtifact-v1",
		canonical_source_identifier,
		source_artifact.sourceCitation(),
		source_artifact.sourceLocator(),
		canonical_media_type,
		"LocalPlantXYZ-ZUp",
		"SI",
		source_artifact.acquisitionMethod() + "; decoded by " + decoder_identifier,
		source_artifact.uncertaintyStatement(),
		payload,
		std::string());
	return VegetationMeasuredSourceArtifactHashService().attachPayloadSha256(
		unsigned_artifact);
}
