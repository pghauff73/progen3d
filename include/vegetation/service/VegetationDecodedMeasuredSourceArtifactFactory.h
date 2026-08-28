#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeContext.h"

#include <json/json.h>

#include <string>

class VegetationDecodedMeasuredSourceArtifactFactory
{
public:
	VegetationMeasuredSourceArtifact create(
		const VegetationMeasuredSourceArtifact &source_artifact,
		const VegetationMeasuredSourceDecodeContext &context,
		const std::string &decoder_identifier,
		const std::string &canonical_media_type,
		Json::Value canonical_document) const;
};
