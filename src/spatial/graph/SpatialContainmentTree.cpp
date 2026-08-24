#include "spatial/graph/SpatialContainmentTree.h"

SpatialContainmentTree::SpatialContainmentTree(
	SpatialObjectId root_object_id,
	std::vector<SpatialContainmentRelationship> relationships)
	: root_object_id_(std::move(root_object_id)),
	  relationships_(std::move(relationships))
{
	for (const SpatialContainmentRelationship &relationship : relationships_) {
		parents_by_child_.emplace(
			relationship.containedObjectId(), relationship.containerId());
		children_by_parent_[relationship.containerId()].push_back(
			relationship.containedObjectId());
	}
}

const SpatialObjectId *SpatialContainmentTree::parentOf(
	const SpatialObjectId &object_id) const
{
	const auto found = parents_by_child_.find(object_id);
	return found == parents_by_child_.end() ? nullptr : &found->second;
}

const std::vector<SpatialObjectId> &SpatialContainmentTree::childrenOf(
	const SpatialObjectId &object_id) const
{
	const auto found = children_by_parent_.find(object_id);
	return found == children_by_parent_.end() ? empty_children_ : found->second;
}
