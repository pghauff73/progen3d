#include "vegetation/service/VegetationMeasuredSourceDecodingService.h"

#include "vegetation/service/VegetationBiomechanicalMaterialTestTableDecoder.h"
#include "vegetation/service/VegetationCanopyObservationTableDecoder.h"
#include "vegetation/service/VegetationMeasuredSourceSchemaRegistry.h"
#include "vegetation/service/VegetationPhenologyObservationTableDecoder.h"
#include "vegetation/service/VegetationRootSystemMarkupLanguageDecoder.h"
#include "vegetation/service/VegetationTreeQsmCylinderTableDecoder.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

VegetationMeasuredSourceDecodeReport rejected_report(
	const VegetationMeasuredSourceArtifact &artifact,
	const std::string &decoder_identifier,
	VegetationMeasuredSourceDecodeIssueCode issue_code,
	const std::string &message)
{
	std::vector<VegetationMeasuredSourceDecodeIssue> issues;
	issues.emplace_back(issue_code, std::string(), message);
	return VegetationMeasuredSourceDecodeReport(
		decoder_identifier,
		artifact.sourceIdentifier(),
		artifact.payloadSha256(),
		std::move(issues),
		{},
		std::nullopt);
}

}

VegetationMeasuredSourceDecodeReport
VegetationMeasuredSourceDecodingService::decode(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationMeasuredSourceDecodeContext &context) const
{
	const VegetationMeasuredSourceSchemaRegistry registry;
	const auto matches = registry.findRegistrations(
		artifact.mediaType(), context.sourceSchemaVersion());
	if (matches.empty()) {
		const bool media_type_registered = std::any_of(
			registry.registrations().begin(),
			registry.registrations().end(),
			[&artifact](const VegetationMeasuredSourceSchemaRegistration &entry) {
				return entry.sourceMediaType() == artifact.mediaType();
			});
		return rejected_report(
			artifact,
			std::string(),
			media_type_registered
				? VegetationMeasuredSourceDecodeIssueCode::
					  UnsupportedSourceSchemaVersion
				: VegetationMeasuredSourceDecodeIssueCode::
					  UnsupportedSourceMediaType,
			media_type_registered
				? "Measured source schema version is not registered."
				: "Measured source media type is not registered.");
	}
	if (matches.size() != 1u) {
		return rejected_report(
			artifact,
			std::string(),
			VegetationMeasuredSourceDecodeIssueCode::
				AmbiguousSourceSchemaRegistration,
			"Measured source media type and schema version resolve to more than one decoder.");
	}
	const std::string &decoder = matches.front().decoderIdentifier();
	if (decoder == "VegetationTreeQsmCylinderTableDecoder") {
		return VegetationTreeQsmCylinderTableDecoder().decode(artifact, context);
	}
	if (decoder == "VegetationRootSystemMarkupLanguageDecoder") {
		return VegetationRootSystemMarkupLanguageDecoder().decode(
			artifact, context);
	}
	if (decoder == "VegetationCanopyObservationTableDecoder") {
		return VegetationCanopyObservationTableDecoder().decode(
			artifact, context);
	}
	if (decoder == "VegetationPhenologyObservationTableDecoder") {
		return VegetationPhenologyObservationTableDecoder().decode(
			artifact, context);
	}
	if (decoder == "VegetationBiomechanicalMaterialTestTableDecoder") {
		return VegetationBiomechanicalMaterialTestTableDecoder().decode(
			artifact, context);
	}
	return rejected_report(
		artifact,
		decoder,
		VegetationMeasuredSourceDecodeIssueCode::CanonicalArtifactCreationFailed,
		"Registered measured-source decoder implementation is unavailable.");
}
