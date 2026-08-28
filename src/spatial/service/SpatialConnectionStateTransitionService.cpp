#include "spatial/service/SpatialConnectionStateTransitionService.h"

bool SpatialConnectionStateTransitionService::canTransition(
	SpatialConnectionState current_state,
	SpatialConnectionState requested_state) const
{
	if (current_state == requested_state) return true;
	if (requested_state == SpatialConnectionState::Invalid ||
	    requested_state == SpatialConnectionState::Broken) {
		return current_state != SpatialConnectionState::Invalid;
	}
	switch (current_state) {
	case SpatialConnectionState::Proposed:
		return requested_state == SpatialConnectionState::Compatible;
	case SpatialConnectionState::Compatible:
		return requested_state == SpatialConnectionState::Positioned;
	case SpatialConnectionState::Positioned:
		return requested_state == SpatialConnectionState::Engaged;
	case SpatialConnectionState::Engaged:
		return requested_state == SpatialConnectionState::Seated ||
		       requested_state == SpatialConnectionState::Locked;
	case SpatialConnectionState::Seated:
		return requested_state == SpatialConnectionState::Locked ||
		       requested_state == SpatialConnectionState::Verified;
	case SpatialConnectionState::Locked:
		return requested_state == SpatialConnectionState::Verified;
	case SpatialConnectionState::Verified:
	case SpatialConnectionState::Broken:
	case SpatialConnectionState::Invalid:
		return false;
	}
	return false;
}
