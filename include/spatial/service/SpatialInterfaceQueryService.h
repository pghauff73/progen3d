#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/model/SpatialInterface.h"
#include "spatial/model/SpatialInterfaceProjection.h"

class SpatialInterfaceQueryService {
public:
	SpatialInterfaceWorldFrame resolveWorldFrame(
		const SpatialInterface &interface,
		const glm::mat4 &object_world_transform) const;

	SpatialInterfaceProjection project(
		const SpatialBuildingObject &object,
		const SpatialInterface &interface,
		const glm::mat4 &object_world_transform,
		const AxisAlignedBounds &object_world_bounds = {}) const;
};
