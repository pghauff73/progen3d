#pragma once

#include "spatial/model/SpatialObjectState.h"

class SpatialObjectStateTransitionService {
public:
	bool canTransition(SpatialObjectState current_state,
	                   SpatialObjectState requested_state) const;
};
