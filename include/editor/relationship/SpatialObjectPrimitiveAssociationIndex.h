#pragma once

#include "spatial/model/SpatialObjectId.h"

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

class SpatialBuildingModel;

class SpatialObjectPrimitiveAssociationIndex
{
public:
	static SpatialObjectPrimitiveAssociationIndex fromModel(
		const SpatialBuildingModel &model);

	std::optional<SpatialObjectId> objectForPrimitive(
		std::size_t primitive_instance_index) const;
	const std::vector<std::size_t> &primitivesForObject(
		const SpatialObjectId &object_id) const;

private:
	std::map<std::size_t, SpatialObjectId> objects_by_primitive_;
	std::map<SpatialObjectId, std::vector<std::size_t>> primitives_by_object_;
	std::vector<std::size_t> empty_primitive_indices_;
};
