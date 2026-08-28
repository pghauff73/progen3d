#pragma once

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceId.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "spatial/model/SpatialInterfaceState.h"
#include "spatial/model/SpatialInterfaceType.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class SpatialInterface {
public:
	SpatialInterface(SpatialInterfaceId interface_id,
	                 SpatialObjectId owner_object_id,
	                 SpatialInterfaceType type,
	                 SpatialInterfaceFrame local_frame,
	                 SpatialInterfaceRegion region,
	                 InterfaceCompatibilityProfile compatibility,
	                 SpatialClearanceRequirement clearance,
	                 SpatialInterfaceState state = SpatialInterfaceState::Available)
		: interface_id_(std::move(interface_id)),
		  owner_object_id_(std::move(owner_object_id)),
		  type_(type),
		  local_frame_(local_frame),
		  region_(std::move(region)),
		  compatibility_(std::move(compatibility)),
		  clearance_(clearance),
		  state_(state) {}

	const SpatialInterfaceId &interfaceId() const { return interface_id_; }
	const SpatialObjectId &ownerObjectId() const { return owner_object_id_; }
	SpatialInterfaceType type() const { return type_; }
	const SpatialInterfaceFrame &localFrame() const { return local_frame_; }
	const SpatialInterfaceRegion &region() const { return region_; }
	const InterfaceCompatibilityProfile &compatibility() const { return compatibility_; }
	const SpatialClearanceRequirement &clearance() const { return clearance_; }
	SpatialInterfaceState state() const { return state_; }

	SpatialInterface withLocalFrame(SpatialInterfaceFrame local_frame) const
	{
		return SpatialInterface(
			interface_id_,
			owner_object_id_,
			type_,
			local_frame,
			region_,
			compatibility_,
			clearance_,
			state_);
	}

	SpatialInterface withOwnerObjectId(SpatialObjectId owner_object_id) const
	{
		return SpatialInterface(
			interface_id_,
			std::move(owner_object_id),
			type_,
			local_frame_,
			region_,
			compatibility_,
			clearance_,
			state_);
	}

private:
	SpatialInterfaceId interface_id_;
	SpatialObjectId owner_object_id_;
	SpatialInterfaceType type_ = SpatialInterfaceType::InspectionInterface;
	SpatialInterfaceFrame local_frame_{
		glm::vec3(0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f)};
	SpatialInterfaceRegion region_ = SpatialInterfaceRegion::point();
	InterfaceCompatibilityProfile compatibility_;
	SpatialClearanceRequirement clearance_{0.0f, 0.0f, 0.0f};
	SpatialInterfaceState state_ = SpatialInterfaceState::Available;
};
