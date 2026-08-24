#pragma once

#include "spatial/model/SpatialInterfaceId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class SpatialInterfaceReference {
public:
	SpatialInterfaceReference(SpatialObjectId object_id, SpatialInterfaceId interface_id)
		: object_id_(std::move(object_id)), interface_id_(std::move(interface_id)) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	const SpatialInterfaceId &interfaceId() const { return interface_id_; }

private:
	SpatialObjectId object_id_;
	SpatialInterfaceId interface_id_;
};
