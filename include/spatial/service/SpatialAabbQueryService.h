#pragma once

#include "geometry/model/AxisAlignedBounds.h"

#include <glm/glm.hpp>

class SpatialAabbQueryService {
public:
	AxisAlignedBounds transform(const AxisAlignedBounds &bounds,
	                            const glm::mat4 &transform) const;
	AxisAlignedBounds translated(const AxisAlignedBounds &bounds,
	                             const glm::vec3 &translation) const;
	AxisAlignedBounds merged(const AxisAlignedBounds &first,
	                         const AxisAlignedBounds &second) const;
	bool overlaps(const AxisAlignedBounds &first,
	              const AxisAlignedBounds &second,
	              float tolerance = 0.0f) const;
	bool touchesOrOverlaps(const AxisAlignedBounds &first,
	                       const AxisAlignedBounds &second,
	                       float tolerance) const;
	float projectedExtent(const AxisAlignedBounds &bounds,
	                      const glm::vec3 &unit_direction) const;
};
