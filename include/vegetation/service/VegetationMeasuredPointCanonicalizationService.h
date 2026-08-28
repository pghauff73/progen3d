#pragma once

#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransform.h"
#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransformResult.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <optional>
#include <string>

class VegetationMeasuredPointCanonicalizationService
{
public:
	VegetationMeasuredCoordinateReferenceTransformResult mapToCanonicalPlantSpace(
		const VegetationMeasuredPoint3d &source_point,
		const std::string &source_coordinate_system,
		const std::string &source_coordinate_unit,
		const std::optional<VegetationMeasuredCoordinateReferenceTransform>
			&coordinate_reference_transform) const;
};
