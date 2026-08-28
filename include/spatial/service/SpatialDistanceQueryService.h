#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/SpatialObjectId.h"
#include "spatial/model/SpatialRelationResult.h"

class SpatialDistanceQueryService {
public:
	SpatialRelationResult measure(const SpatialObjectId &first_object_id,
	                              const AxisAlignedBounds &first,
	                              const SpatialObjectId &second_object_id,
	                              const AxisAlignedBounds &second,
	                              float tolerance = 0.000001f) const;
};
