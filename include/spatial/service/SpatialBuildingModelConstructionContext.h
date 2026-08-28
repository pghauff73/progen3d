#pragma once

#include "spatial/model/SpatialBuildingModelConstructionResult.h"
#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/model/SpatialConnection.h"
#include "spatial/model/SpatialModelSafetyLimits.h"
#include "spatial/relationship/SpatialObjectGeometryBinding.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class SpatialConstraint;

class SpatialBuildingModelConstructionContext {
public:
	SpatialBuildingModelConstructionContext();
	~SpatialBuildingModelConstructionContext();

	SpatialBuildingModelConstructionContext(
		const SpatialBuildingModelConstructionContext &) = delete;
	SpatialBuildingModelConstructionContext &operator=(
		const SpatialBuildingModelConstructionContext &) = delete;

	bool beginObject(SpatialObjectIdentity identity,
	                 SpatialFrameState frame_state,
	                 SpatialBoundaryModel boundary_model,
	                 CollisionParticipationPolicy collision_policy,
	                 SpatialObjectState initial_state,
	                 std::string *diagnostic);
	bool addInterface(SpatialInterface interface, std::string *diagnostic);
	bool bindPrimitive(std::size_t primitive_instance_index,
	                   glm::mat4 primitive_local_to_object_transform,
	                   std::vector<MeshSurfaceTag> bound_surface_tags,
	                   std::string *diagnostic);
	bool endObject(std::string *diagnostic);

	void addObject(SpatialBuildingObject object);
	void setRootObjectId(SpatialObjectId root_object_id);
	void addContainmentRelationship(SpatialContainmentRelationship relationship);
	void addConnection(SpatialConnection connection);
	void addConstraint(std::shared_ptr<const SpatialConstraint> constraint);
	void addResolutionRecord(SpatialResolutionRecord record);

	bool hasOpenObjectScopes() const;
	SpatialBuildingModelConstructionResult finalize(
		std::optional<std::size_t> primitive_instance_count = std::nullopt,
		const SpatialModelSafetyLimits &limits = SpatialModelSafetyLimits()) const;

private:
	class ConstructionState;
	std::unique_ptr<ConstructionState> state_;
};
