#pragma once

#include "spatial/relationship/SpatialContainmentRelationship.h"

#include <map>
#include <utility>
#include <vector>

class SpatialContainmentTree {
public:
	SpatialContainmentTree() = default;
	SpatialContainmentTree(SpatialObjectId root_object_id,
	                       std::vector<SpatialContainmentRelationship> relationships);

	const SpatialObjectId &rootObjectId() const { return root_object_id_; }
	const SpatialObjectId *parentOf(const SpatialObjectId &object_id) const;
	const std::vector<SpatialObjectId> &childrenOf(const SpatialObjectId &object_id) const;
	const std::vector<SpatialContainmentRelationship> &relationships() const
	{
		return relationships_;
	}

private:
	SpatialObjectId root_object_id_;
	std::vector<SpatialContainmentRelationship> relationships_;
	std::map<SpatialObjectId, SpatialObjectId> parents_by_child_;
	std::map<SpatialObjectId, std::vector<SpatialObjectId>> children_by_parent_;
	std::vector<SpatialObjectId> empty_children_;
};
