#pragma once

#include "spatial/model/CollisionParticipationPolicy.h"
#include "spatial/model/SpatialBoundaryModel.h"
#include "spatial/model/SpatialFrameState.h"
#include "spatial/model/SpatialInterface.h"
#include "spatial/model/SpatialObjectIdentity.h"
#include "spatial/model/SpatialObjectState.h"

#include "mechanics/model/PinRollerMember.h"
#include <memory>
#include <vector>

#include <utility>

class SpatialBuildingObject {
public:
	SpatialBuildingObject(SpatialObjectIdentity identity,
	                      SpatialFrameState frame_state,
	                      SpatialBoundaryModel boundary_model,
	                      std::vector<SpatialInterface> interfaces,
	                      CollisionParticipationPolicy collision_policy,
	                      SpatialObjectState state,
	                      std::shared_ptr<const PinRollerMember> member = {})
		: identity_(std::move(identity)),
		  frame_state_(std::move(frame_state)),
		  boundary_model_(std::move(boundary_model)),
		  interfaces_(std::move(interfaces)),
		  collision_policy_(collision_policy),
		  state_(state),
		  member_(std::move(member)) {}

	const std::shared_ptr<const PinRollerMember> &pinRollerMember() const { return member_; }
	const SpatialObjectIdentity &identity() const { return identity_; }
	const SpatialFrameState &frameState() const { return frame_state_; }
	const SpatialBoundaryModel &boundaryModel() const { return boundary_model_; }
	const std::vector<SpatialInterface> &interfaces() const { return interfaces_; }
	const CollisionParticipationPolicy &collisionPolicy() const { return collision_policy_; }
	SpatialObjectState state() const { return state_; }

	const SpatialInterface *findInterface(const SpatialInterfaceId &interface_id) const
	{
		for (const SpatialInterface &interface : interfaces_) {
			if (interface.interfaceId() == interface_id) return &interface;
		}
		return nullptr;
	}

	SpatialBuildingObject withFrameState(SpatialFrameState frame_state,
	                                     SpatialObjectState state) const
	{
		return SpatialBuildingObject(
			identity_,
			std::move(frame_state),
			boundary_model_,
			interfaces_,
			collision_policy_,
			state, member_);
	}

	SpatialBuildingObject withBoundaryModel(SpatialBoundaryModel boundary_model) const
	{
		return SpatialBuildingObject(
			identity_,
			frame_state_,
			std::move(boundary_model),
			interfaces_,
			collision_policy_,
			state_, member_);
	}

	SpatialBuildingObject withInterfaces(std::vector<SpatialInterface> interfaces) const
	{
		return SpatialBuildingObject(
			identity_,
			frame_state_,
			boundary_model_,
			std::move(interfaces),
			collision_policy_,
			state_, member_);
	}

private:
	SpatialObjectIdentity identity_;
	SpatialFrameState frame_state_;
	SpatialBoundaryModel boundary_model_;
	std::vector<SpatialInterface> interfaces_;
	CollisionParticipationPolicy collision_policy_{
		CollisionLayer::Temporary, CollisionLayerMask()};
	SpatialObjectState state_ = SpatialObjectState::Unplaced;
	std::shared_ptr<const PinRollerMember> member_;
};
