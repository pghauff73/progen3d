#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

#include <string>

class VegetationMeasuredSourceArtifactHashService
{
public:
	std::string calculatePayloadSha256(
		const VegetationMeasuredSourceArtifact &artifact) const;
	VegetationMeasuredSourceArtifact attachPayloadSha256(
		const VegetationMeasuredSourceArtifact &artifact) const;
};
