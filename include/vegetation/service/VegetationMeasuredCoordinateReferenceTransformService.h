#pragma once

#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransform.h"
#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransformResult.h"
#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransformValidationReport.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"

class VegetationMeasuredCoordinateReferenceTransformService
{
public:
	VegetationMeasuredCoordinateReferenceTransformValidationReport validate(
		const VegetationMeasuredCoordinateReferenceTransform &transform) const;
	VegetationMeasuredCoordinateReferenceTransformResult mapSourceToTarget(
		const VegetationMeasuredPoint3d &source_point,
		const VegetationMeasuredCoordinateReferenceTransform &transform) const;
	VegetationMeasuredCoordinateReferenceTransformResult mapTargetToSource(
		const VegetationMeasuredPoint3d &target_point,
		const VegetationMeasuredCoordinateReferenceTransform &transform) const;
};
