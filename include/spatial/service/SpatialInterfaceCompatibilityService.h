#pragma once

#include "spatial/model/SpatialInterface.h"
#include "spatial/model/SpatialInterfaceCompatibilityResult.h"

class SpatialInterfaceCompatibilityService {
public:
	SpatialInterfaceCompatibilityResult evaluate(
		const SpatialInterface &source,
		const SpatialInterface &target,
		float dimensional_tolerance) const;
};
