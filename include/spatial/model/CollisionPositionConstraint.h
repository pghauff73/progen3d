#pragma once

#include "spatial/model/CollisionLayerMask.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialConstraint.h"
#include "spatial/model/SpatialDirection.h"
#include "spatial/model/SpatialInterfaceId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

enum class CollisionPositionMode {
	Touch,
	Gap,
	Drop,
	Seat,
	Insert,
	Tangent,
	Between,
	CenterContact
};

class CollisionPositionConstraint : public SpatialConstraint {
public:
	CollisionPositionConstraint(SpatialConstraintId constraint_id,
	                            SpatialObjectId moving_object_id,
	                            SpatialInterfaceId moving_interface_id,
	                            SpatialObjectId target_object_id,
	                            SpatialInterfaceId target_interface_id,
	                            SpatialDirection direction,
	                            CollisionPositionMode mode,
	                            SpatialClearanceRequirement clearance,
	                            float seating_depth,
	                            float maximum_distance,
	                            float tolerance,
	                            CollisionLayerMask collision_mask,
	                            int priority)
		: constraint_id_(std::move(constraint_id)),
		  moving_object_id_(std::move(moving_object_id)),
		  moving_interface_id_(std::move(moving_interface_id)),
		  target_object_id_(std::move(target_object_id)),
		  target_interface_id_(std::move(target_interface_id)),
		  direction_(direction),
		  mode_(mode),
		  clearance_(clearance),
		  seating_depth_(seating_depth),
		  maximum_distance_(maximum_distance),
		  tolerance_(tolerance),
		  collision_mask_(collision_mask),
		  priority_(priority) {}

	SpatialConstraintKind kind() const override
	{
		return SpatialConstraintKind::CollisionPosition;
	}
	const SpatialConstraintId &constraintId() const override { return constraint_id_; }
	int priority() const override { return priority_; }

	const SpatialObjectId &movingObjectId() const { return moving_object_id_; }
	const SpatialInterfaceId &movingInterfaceId() const { return moving_interface_id_; }
	const SpatialObjectId &targetObjectId() const { return target_object_id_; }
	const SpatialInterfaceId &targetInterfaceId() const { return target_interface_id_; }
	const SpatialDirection &direction() const { return direction_; }
	CollisionPositionMode mode() const { return mode_; }
	const SpatialClearanceRequirement &clearance() const { return clearance_; }
	float seatingDepth() const { return seating_depth_; }
	float maximumDistance() const { return maximum_distance_; }
	float tolerance() const { return tolerance_; }
	const CollisionLayerMask &collisionMask() const { return collision_mask_; }

private:
	SpatialConstraintId constraint_id_;
	SpatialObjectId moving_object_id_;
	SpatialInterfaceId moving_interface_id_;
	SpatialObjectId target_object_id_;
	SpatialInterfaceId target_interface_id_;
	SpatialDirection direction_{SpatialDirectionFrame::World, glm::vec3(0.0f)};
	CollisionPositionMode mode_ = CollisionPositionMode::Touch;
	SpatialClearanceRequirement clearance_{0.0f, 0.0f, 0.0f};
	float seating_depth_ = 0.0f;
	float maximum_distance_ = 0.0f;
	float tolerance_ = 0.0005f;
	CollisionLayerMask collision_mask_;
	int priority_ = 0;
};
