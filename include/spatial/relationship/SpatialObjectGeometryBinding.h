#pragma once

#include "geometry/model/MeshSurfaceTag.h"
#include "spatial/model/SpatialObjectId.h"

#include <cstddef>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

class SpatialObjectGeometryBinding {
public:
	SpatialObjectGeometryBinding(SpatialObjectId object_id,
	                             std::size_t primitive_instance_index,
	                             glm::mat4 primitive_local_to_object_transform,
	                             std::vector<MeshSurfaceTag> bound_surface_tags = {})
		: object_id_(std::move(object_id)),
		  primitive_instance_index_(primitive_instance_index),
		  primitive_local_to_object_transform_(primitive_local_to_object_transform),
		  bound_surface_tags_(std::move(bound_surface_tags)) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	std::size_t primitiveInstanceIndex() const { return primitive_instance_index_; }
	const glm::mat4 &primitiveLocalToObjectTransform() const
	{
		return primitive_local_to_object_transform_;
	}
	const std::vector<MeshSurfaceTag> &boundSurfaceTags() const { return bound_surface_tags_; }

private:
	SpatialObjectId object_id_;
	std::size_t primitive_instance_index_ = 0;
	glm::mat4 primitive_local_to_object_transform_{1.0f};
	std::vector<MeshSurfaceTag> bound_surface_tags_;
};
