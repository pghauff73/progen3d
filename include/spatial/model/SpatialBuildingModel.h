#pragma once

#include "spatial/graph/SpatialConnectionGraph.h"
#include "spatial/graph/SpatialConstraintGraph.h"
#include "spatial/graph/SpatialContainmentTree.h"
#include "spatial/graph/SpatialObjectRegistry.h"
#include "spatial/model/SpatialResolutionRecord.h"
#include "spatial/relationship/SpatialObjectGeometryBinding.h"

#include <vector>

#include <utility>

class SpatialBuildingModel {
public:
	SpatialBuildingModel(SpatialObjectRegistry objects,
	                     SpatialContainmentTree containment_tree,
	                     SpatialConnectionGraph connection_graph,
	                     SpatialConstraintGraph constraint_graph,
	                     std::vector<SpatialObjectGeometryBinding> geometry_bindings,
	                     std::vector<SpatialResolutionRecord> resolution_records)
		: objects_(std::move(objects)),
		  containment_tree_(std::move(containment_tree)),
		  connection_graph_(std::move(connection_graph)),
		  constraint_graph_(std::move(constraint_graph)),
		  geometry_bindings_(std::move(geometry_bindings)),
		  resolution_records_(std::move(resolution_records)) {}

	const SpatialObjectRegistry &objects() const { return objects_; }
	const SpatialContainmentTree &containmentTree() const { return containment_tree_; }
	const SpatialConnectionGraph &connectionGraph() const { return connection_graph_; }
	const SpatialConstraintGraph &constraintGraph() const { return constraint_graph_; }
	const std::vector<SpatialObjectGeometryBinding> &geometryBindings() const
	{
		return geometry_bindings_;
	}
	const std::vector<SpatialResolutionRecord> &resolutionRecords() const
	{
		return resolution_records_;
	}

	std::vector<SpatialObjectGeometryBinding> bindingsFor(
		const SpatialObjectId &object_id) const;

	SpatialBuildingModel replacingObject(const SpatialBuildingObject &object) const;
	SpatialBuildingModel appendingResolutionRecord(SpatialResolutionRecord record) const;

private:
	SpatialObjectRegistry objects_;
	SpatialContainmentTree containment_tree_;
	SpatialConnectionGraph connection_graph_;
	SpatialConstraintGraph constraint_graph_;
	std::vector<SpatialObjectGeometryBinding> geometry_bindings_;
	std::vector<SpatialResolutionRecord> resolution_records_;
};
