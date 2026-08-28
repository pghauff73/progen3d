#pragma once

#include "geometry/model/AxisAlignedBounds.h"

class SpatialContainmentQueryService {
public:
	bool contains(const AxisAlignedBounds &container,
	              const AxisAlignedBounds &contained,
	              float tolerance = 0.000001f) const;
	bool isAbove(const AxisAlignedBounds &first,
	             const AxisAlignedBounds &second,
	             float tolerance = 0.000001f) const;
	bool isBelow(const AxisAlignedBounds &first,
	             const AxisAlignedBounds &second,
	             float tolerance = 0.000001f) const;
};
