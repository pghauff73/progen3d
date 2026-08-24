#include "spatial/model/SpatialBuildingModel.h"

std::vector<SpatialObjectGeometryBinding> SpatialBuildingModel::bindingsFor(
	const SpatialObjectId &object_id) const
{
	std::vector<SpatialObjectGeometryBinding> matches;
	for (const SpatialObjectGeometryBinding &binding : geometry_bindings_) {
		if (binding.objectId() == object_id) matches.push_back(binding);
	}
	return matches;
}

SpatialBuildingModel SpatialBuildingModel::replacingObject(
	const SpatialBuildingObject &object) const
{
	return SpatialBuildingModel(
		objects_.replacing(object),
		containment_tree_,
		connection_graph_,
		constraint_graph_,
		geometry_bindings_,
		resolution_records_);
}

SpatialBuildingModel SpatialBuildingModel::appendingResolutionRecord(
	SpatialResolutionRecord record) const
{
	std::vector<SpatialResolutionRecord> records = resolution_records_;
	records.push_back(std::move(record));
	return SpatialBuildingModel(
		objects_,
		containment_tree_,
		connection_graph_,
		constraint_graph_,
		geometry_bindings_,
		std::move(records));
}
