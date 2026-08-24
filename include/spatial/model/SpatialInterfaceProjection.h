#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/SpatialInterfaceWorldFrame.h"

class SpatialInterfaceProjection {
public:
	SpatialInterfaceProjection(SpatialInterfaceWorldFrame world_frame,
	                           AxisAlignedBounds world_region_bounds)
		: world_frame_(world_frame),
		  world_region_bounds_(world_region_bounds) {}

	const SpatialInterfaceWorldFrame &worldFrame() const { return world_frame_; }
	const AxisAlignedBounds &worldRegionBounds() const { return world_region_bounds_; }

private:
	SpatialInterfaceWorldFrame world_frame_{{0.0f, 0.0f, 0.0f},
	                                        {0.0f, 1.0f, 0.0f},
	                                        {1.0f, 0.0f, 0.0f},
	                                        {0.0f, 0.0f, 1.0f}};
	AxisAlignedBounds world_region_bounds_;
};
