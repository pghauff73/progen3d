#pragma once

#include "building/model/BuildingCollisionBehaviorKind.h"
#include "building/model/BuildingConnectionPoint.h"
#include "building/model/BuildingObjectExtentSet.h"
#include "building/model/BuildingOrientationProfile.h"
#include "building/model/BuildingPlacementPolicyKind.h"
#include "building/model/BuildingSpatialManifestationKind.h"
#include "spatial/model/SpatialObjectId.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class BuildingObjectSpatialProfile {
public:
	BuildingObjectSpatialProfile(
		SpatialObjectId object_id,
		BuildingSpatialManifestationKind manifestation_kind,
		BuildingPlacementPolicyKind placement_policy,
		BuildingCollisionBehaviorKind collision_behavior,
		BuildingObjectExtentSet extent_set,
		BuildingOrientationProfile orientation_profile,
		std::vector<BuildingConnectionPoint> connection_points,
		std::vector<std::string> graph_memberships,
		std::uint64_t profile_revision,
		std::uint64_t profile_hash)
		: object_id_(std::move(object_id)),
		  manifestation_kind_(manifestation_kind),
		  placement_policy_(placement_policy),
		  collision_behavior_(collision_behavior),
		  extent_set_(std::move(extent_set)),
		  orientation_profile_(std::move(orientation_profile)),
		  connection_points_(std::move(connection_points)),
		  graph_memberships_(std::move(graph_memberships)),
		  profile_revision_(profile_revision),
		  profile_hash_(profile_hash) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	BuildingSpatialManifestationKind manifestationKind() const
	{
		return manifestation_kind_;
	}
	BuildingPlacementPolicyKind placementPolicy() const { return placement_policy_; }
	BuildingCollisionBehaviorKind collisionBehavior() const { return collision_behavior_; }
	const BuildingObjectExtentSet &extentSet() const { return extent_set_; }
	const BuildingOrientationProfile &orientationProfile() const
	{
		return orientation_profile_;
	}
	const std::vector<BuildingConnectionPoint> &connectionPoints() const
	{
		return connection_points_;
	}
	const std::vector<std::string> &graphMemberships() const
	{
		return graph_memberships_;
	}
	std::uint64_t profileRevision() const { return profile_revision_; }
	std::uint64_t profileHash() const { return profile_hash_; }
	bool isSpatiallyApplicable() const
	{
		return manifestation_kind_ != BuildingSpatialManifestationKind::SemanticOnly;
	}

	BuildingObjectSpatialProfile withProfileHash(std::uint64_t profile_hash) const
	{
		return BuildingObjectSpatialProfile(
			object_id_, manifestation_kind_, placement_policy_, collision_behavior_,
			extent_set_, orientation_profile_, connection_points_, graph_memberships_,
			profile_revision_, profile_hash);
	}

private:
	SpatialObjectId object_id_;
	BuildingSpatialManifestationKind manifestation_kind_ =
		BuildingSpatialManifestationKind::SemanticOnly;
	BuildingPlacementPolicyKind placement_policy_ =
		BuildingPlacementPolicyKind::NotApplicable;
	BuildingCollisionBehaviorKind collision_behavior_ =
		BuildingCollisionBehaviorKind::NonParticipating;
	BuildingObjectExtentSet extent_set_;
	BuildingOrientationProfile orientation_profile_ =
		BuildingOrientationProfile::notApplicable();
	std::vector<BuildingConnectionPoint> connection_points_;
	std::vector<std::string> graph_memberships_;
	std::uint64_t profile_revision_ = 0;
	std::uint64_t profile_hash_ = 0;
};
