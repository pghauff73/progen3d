#include "spatial/service/SpatialObjectStateTransitionService.h"

bool SpatialObjectStateTransitionService::canTransition(
	SpatialObjectState current_state,
	SpatialObjectState requested_state) const
{
	if (current_state == requested_state) return true;
	if (requested_state == SpatialObjectState::Invalid) return true;
	switch (current_state) {
	case SpatialObjectState::Unplaced:
		return requested_state == SpatialObjectState::Approximate;
	case SpatialObjectState::Approximate:
		return requested_state == SpatialObjectState::Positioned;
	case SpatialObjectState::Positioned:
		return requested_state == SpatialObjectState::ContactResolved;
	case SpatialObjectState::ContactResolved:
		return requested_state == SpatialObjectState::Connected ||
		       requested_state == SpatialObjectState::Constrained;
	case SpatialObjectState::Connected:
		return requested_state == SpatialObjectState::Constrained;
	case SpatialObjectState::Constrained:
		return requested_state == SpatialObjectState::Validated;
	case SpatialObjectState::Validated:
	case SpatialObjectState::Invalid:
		return false;
	}
	return false;
}
