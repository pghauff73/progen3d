#pragma once

#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/model/SpatialConnection.h"
#include "spatial/model/SpatialResolutionRecord.h"
#include "spatial/relationship/SpatialContainmentRelationship.h"
#include "spatial/relationship/SpatialObjectGeometryBinding.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class SpatialConstraint;

class SpatialBuildingModelConstructionRequest {
public:
	SpatialBuildingModelConstructionRequest(
		std::vector<SpatialBuildingObject> objects,
		SpatialObjectId root_object_id,
		std::vector<SpatialContainmentRelationship> containment_relationships,
		std::vector<SpatialObjectGeometryBinding> geometry_bindings,
		std::vector<SpatialConnection> connections,
		std::vector<std::shared_ptr<const SpatialConstraint>> constraints,
		std::vector<SpatialResolutionRecord> resolution_records,
		std::optional<std::size_t> primitive_instance_count = std::nullopt)
		: objects_(std::move(objects)),
		  root_object_id_(std::move(root_object_id)),
		  containment_relationships_(std::move(containment_relationships)),
		  geometry_bindings_(std::move(geometry_bindings)),
		  connections_(std::move(connections)),
		  constraints_(std::move(constraints)),
		  resolution_records_(std::move(resolution_records)),
		  primitive_instance_count_(primitive_instance_count) {}

	const std::vector<SpatialBuildingObject> &objects() const { return objects_; }
	const SpatialObjectId &rootObjectId() const { return root_object_id_; }
	const std::vector<SpatialContainmentRelationship> &containmentRelationships() const
	{
		return containment_relationships_;
	}
	const std::vector<SpatialObjectGeometryBinding> &geometryBindings() const
	{
		return geometry_bindings_;
	}
	const std::vector<SpatialConnection> &connections() const { return connections_; }
	const std::vector<std::shared_ptr<const SpatialConstraint>> &constraints() const
	{
		return constraints_;
	}
	const std::vector<SpatialResolutionRecord> &resolutionRecords() const
	{
		return resolution_records_;
	}
	const std::optional<std::size_t> &primitiveInstanceCount() const
	{
		return primitive_instance_count_;
	}

private:
	std::vector<SpatialBuildingObject> objects_;
	SpatialObjectId root_object_id_;
	std::vector<SpatialContainmentRelationship> containment_relationships_;
	std::vector<SpatialObjectGeometryBinding> geometry_bindings_;
	std::vector<SpatialConnection> connections_;
	std::vector<std::shared_ptr<const SpatialConstraint>> constraints_;
	std::vector<SpatialResolutionRecord> resolution_records_;
	std::optional<std::size_t> primitive_instance_count_;
};
