#pragma once

#include "spatial/model/SpatialConnectionState.h"

class SpatialConnectionStateTransitionService {
public:
	bool canTransition(SpatialConnectionState current_state,
	                   SpatialConnectionState requested_state) const;
};
