#include "editor/relationship/SpatialObjectPrimitiveAssociationIndex.h"

#include "spatial/model/SpatialBuildingModel.h"

SpatialObjectPrimitiveAssociationIndex
SpatialObjectPrimitiveAssociationIndex::fromModel(const SpatialBuildingModel &model)
{
	SpatialObjectPrimitiveAssociationIndex index;
	for (const SpatialObjectGeometryBinding &binding : model.geometryBindings()) {
		index.objects_by_primitive_.emplace(
			binding.primitiveInstanceIndex(), binding.objectId());
		index.primitives_by_object_[binding.objectId()].push_back(
			binding.primitiveInstanceIndex());
	}
	return index;
}

std::optional<SpatialObjectId>
SpatialObjectPrimitiveAssociationIndex::objectForPrimitive(
	std::size_t primitive_instance_index) const
{
	const auto found = objects_by_primitive_.find(primitive_instance_index);
	if (found == objects_by_primitive_.end()) return std::nullopt;
	return found->second;
}

const std::vector<std::size_t> &
SpatialObjectPrimitiveAssociationIndex::primitivesForObject(
	const SpatialObjectId &object_id) const
{
	const auto found = primitives_by_object_.find(object_id);
	return found == primitives_by_object_.end()
		? empty_primitive_indices_
		: found->second;
}
