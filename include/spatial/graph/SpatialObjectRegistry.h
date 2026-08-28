#pragma once

#include "spatial/model/SpatialBuildingObject.h"

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

class SpatialObjectRegistry {
public:
	SpatialObjectRegistry() = default;
	explicit SpatialObjectRegistry(std::vector<SpatialBuildingObject> objects);

	const std::vector<SpatialBuildingObject> &objects() const { return objects_; }
	const SpatialBuildingObject *find(const SpatialObjectId &object_id) const;
	std::size_t size() const { return objects_.size(); }

	SpatialObjectRegistry replacing(const SpatialBuildingObject &object) const;

private:
	std::vector<SpatialBuildingObject> objects_;
	std::map<SpatialObjectId, std::size_t> indices_by_id_;
};
