#include "spatial/service/SpatialBuildingModelConstructionContext.h"

#include "spatial/model/SpatialBuildingModelConstructionRequest.h"
#include "spatial/service/SpatialBuildingModelConstructionService.h"

#include <algorithm>
#include <utility>

class SpatialBuildingModelConstructionContext::ConstructionState {
public:
	struct PendingObjectDeclaration {
		std::size_t declaration_order = 0;
		SpatialObjectIdentity identity;
		SpatialFrameState frame_state;
		SpatialBoundaryModel boundary_model;
		CollisionParticipationPolicy collision_policy;
		SpatialObjectState state = SpatialObjectState::Unplaced;
		std::vector<SpatialInterface> interfaces;
	};

	struct OrderedObject {
		std::size_t declaration_order = 0;
		SpatialBuildingObject object;
	};

	std::size_t next_declaration_order = 0;
	std::vector<PendingObjectDeclaration> object_stack;
	std::vector<OrderedObject> objects;
	SpatialObjectId root_object_id;
	std::vector<SpatialContainmentRelationship> containment_relationships;
	std::vector<SpatialObjectGeometryBinding> geometry_bindings;
	std::vector<SpatialConnection> connections;
	std::vector<std::shared_ptr<const SpatialConstraint>> constraints;
	std::vector<SpatialResolutionRecord> resolution_records;
};

SpatialBuildingModelConstructionContext::SpatialBuildingModelConstructionContext()
	: state_(std::make_unique<ConstructionState>())
{
}

SpatialBuildingModelConstructionContext::~SpatialBuildingModelConstructionContext() = default;

bool SpatialBuildingModelConstructionContext::beginObject(
	SpatialObjectIdentity identity,
	SpatialFrameState frame_state,
	SpatialBoundaryModel boundary_model,
	CollisionParticipationPolicy collision_policy,
	SpatialObjectState initial_state,
	std::string *diagnostic)
{
	if (identity.objectId().empty()) {
		if (diagnostic != nullptr) *diagnostic = "Spatial object ID must not be empty.";
		return false;
	}
	if (state_->object_stack.empty()) {
		if (state_->root_object_id.empty()) state_->root_object_id = identity.objectId();
	} else {
		state_->containment_relationships.emplace_back(
			state_->object_stack.back().identity.objectId(), identity.objectId());
	}
	state_->object_stack.push_back(ConstructionState::PendingObjectDeclaration{
		state_->next_declaration_order++,
		std::move(identity),
		std::move(frame_state),
		std::move(boundary_model),
		collision_policy,
		initial_state,
		{}});
	return true;
}

bool SpatialBuildingModelConstructionContext::addInterface(
	SpatialInterface interface,
	std::string *diagnostic)
{
	if (state_->object_stack.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Spatial interface declarations require an active object scope.";
		}
		return false;
	}
	state_->object_stack.back().interfaces.push_back(std::move(interface));
	return true;
}

bool SpatialBuildingModelConstructionContext::bindPrimitive(
	std::size_t primitive_instance_index,
	glm::mat4 primitive_local_to_object_transform,
	std::vector<MeshSurfaceTag> bound_surface_tags,
	std::string *diagnostic)
{
	if (state_->object_stack.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Primitive bindings require an active spatial object scope.";
		}
		return false;
	}
	state_->geometry_bindings.emplace_back(
		state_->object_stack.back().identity.objectId(),
		primitive_instance_index,
		primitive_local_to_object_transform,
		std::move(bound_surface_tags));
	return true;
}

bool SpatialBuildingModelConstructionContext::endObject(std::string *diagnostic)
{
	if (state_->object_stack.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "No spatial object scope is available to close.";
		}
		return false;
	}
	ConstructionState::PendingObjectDeclaration declaration =
		std::move(state_->object_stack.back());
	state_->object_stack.pop_back();
	state_->objects.push_back(ConstructionState::OrderedObject{
		declaration.declaration_order,
		SpatialBuildingObject(
			std::move(declaration.identity),
			std::move(declaration.frame_state),
			std::move(declaration.boundary_model),
			std::move(declaration.interfaces),
			declaration.collision_policy,
			declaration.state)});
	return true;
}

void SpatialBuildingModelConstructionContext::addObject(SpatialBuildingObject object)
{
	state_->objects.push_back(ConstructionState::OrderedObject{
		state_->next_declaration_order++, std::move(object)});
}

void SpatialBuildingModelConstructionContext::setRootObjectId(
	SpatialObjectId root_object_id)
{
	state_->root_object_id = std::move(root_object_id);
}

void SpatialBuildingModelConstructionContext::addContainmentRelationship(
	SpatialContainmentRelationship relationship)
{
	state_->containment_relationships.push_back(std::move(relationship));
}

void SpatialBuildingModelConstructionContext::addConnection(
	SpatialConnection connection)
{
	state_->connections.push_back(std::move(connection));
}

void SpatialBuildingModelConstructionContext::addConstraint(
	std::shared_ptr<const SpatialConstraint> constraint)
{
	state_->constraints.push_back(std::move(constraint));
}

void SpatialBuildingModelConstructionContext::addResolutionRecord(
	SpatialResolutionRecord record)
{
	state_->resolution_records.push_back(std::move(record));
}

bool SpatialBuildingModelConstructionContext::hasOpenObjectScopes() const
{
	return !state_->object_stack.empty();
}

SpatialBuildingModelConstructionResult SpatialBuildingModelConstructionContext::finalize(
	std::optional<std::size_t> primitive_instance_count,
	const SpatialModelSafetyLimits &limits) const
{
	if (hasOpenObjectScopes()) {
		SpatialModelValidationReport report;
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::InvalidIdentity,
			"Spatial model construction ended with unclosed object scopes."));
		return SpatialBuildingModelConstructionResult(nullptr, std::move(report));
	}

	std::vector<ConstructionState::OrderedObject> ordered_objects = state_->objects;
	std::sort(ordered_objects.begin(), ordered_objects.end(),
	          [](const ConstructionState::OrderedObject &left,
	             const ConstructionState::OrderedObject &right) {
		          return left.declaration_order < right.declaration_order;
	          });
	std::vector<SpatialBuildingObject> objects;
	objects.reserve(ordered_objects.size());
	for (const ConstructionState::OrderedObject &ordered_object : ordered_objects) {
		objects.push_back(ordered_object.object);
	}

	SpatialBuildingModelConstructionRequest request(
		std::move(objects),
		state_->root_object_id,
		state_->containment_relationships,
		state_->geometry_bindings,
		state_->connections,
		state_->constraints,
		state_->resolution_records,
		primitive_instance_count);
	return SpatialBuildingModelConstructionService().construct(request, limits);
}
