#pragma once

#include "spatial/model/SpatialObjectId.h"

#include <utility>

class SpatialContainmentRelationship {
public:
	SpatialContainmentRelationship(SpatialObjectId container_id,
	                               SpatialObjectId contained_object_id)
		: container_id_(std::move(container_id)),
		  contained_object_id_(std::move(contained_object_id)) {}

	const SpatialObjectId &containerId() const { return container_id_; }
	const SpatialObjectId &containedObjectId() const { return contained_object_id_; }

private:
	SpatialObjectId container_id_;
	SpatialObjectId contained_object_id_;
};
