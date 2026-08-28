#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeContext.h"
#include "vegetation/model/VegetationMeasuredSourceDecodeReport.h"

class VegetationBiomechanicalMaterialTestTableDecoder
{
public:
	VegetationMeasuredSourceDecodeReport decode(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationMeasuredSourceDecodeContext &context) const;
};
