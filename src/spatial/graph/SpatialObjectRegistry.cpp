#include "spatial/graph/SpatialObjectRegistry.h"

SpatialObjectRegistry::SpatialObjectRegistry(std::vector<SpatialBuildingObject> objects)
	: objects_(std::move(objects))
{
	for (std::size_t index = 0; index < objects_.size(); ++index) {
		indices_by_id_.emplace(objects_[index].identity().objectId(), index);
	}
}

const SpatialBuildingObject *SpatialObjectRegistry::find(const SpatialObjectId &object_id) const
{
	const auto found = indices_by_id_.find(object_id);
	return found == indices_by_id_.end() ? nullptr : &objects_[found->second];
}

SpatialObjectRegistry SpatialObjectRegistry::replacing(const SpatialBuildingObject &object) const
{
	std::vector<SpatialBuildingObject> replaced_objects = objects_;
	const auto found = indices_by_id_.find(object.identity().objectId());
	if (found != indices_by_id_.end()) {
		replaced_objects[found->second] = object;
	}
	return SpatialObjectRegistry(std::move(replaced_objects));
}
