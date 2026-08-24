#include "editor/service/SpatialObjectInspectionService.h"

#include "editor/service/BuildingKnowledgeInspectionService.h"
#include "editor/relationship/SpatialObjectPrimitiveAssociationIndex.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {

std::array<float, 3> vector3_values(const glm::vec3 &value)
{
	return {value.x, value.y, value.z};
}

std::array<float, 16> matrix_values(const glm::mat4 &matrix)
{
	std::array<float, 16> values{};
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			values[static_cast<std::size_t>(column * 4 + row)] = matrix[column][row];
		}
	}
	return values;
}

std::string join_taxonomy(const SpatialTaxonomyPath &taxonomy)
{
	std::ostringstream text;
	for (std::size_t index = 0; index < taxonomy.segments().size(); ++index) {
		if (index > 0) text << " > ";
		text << taxonomy.segments()[index];
	}
	return text.str();
}

const char *object_state_name(SpatialObjectState state)
{
	switch (state) {
	case SpatialObjectState::Unplaced: return "Unplaced";
	case SpatialObjectState::Approximate: return "Approximate";
	case SpatialObjectState::Positioned: return "Positioned";
	case SpatialObjectState::ContactResolved: return "Contact resolved";
	case SpatialObjectState::Connected: return "Connected";
	case SpatialObjectState::Constrained: return "Constrained";
	case SpatialObjectState::Validated: return "Validated";
	case SpatialObjectState::Invalid: return "Invalid";
	}
	return "Unknown";
}

const char *interface_type_name(SpatialInterfaceType type)
{
	switch (type) {
	case SpatialInterfaceType::Support: return "Support";
	case SpatialInterfaceType::Bearing: return "Bearing";
	case SpatialInterfaceType::Mate: return "Mate";
	case SpatialInterfaceType::Seat: return "Seat";
	case SpatialInterfaceType::Insert: return "Insert";
	case SpatialInterfaceType::Socket: return "Socket";
	case SpatialInterfaceType::Shaft: return "Shaft";
	case SpatialInterfaceType::Seal: return "Seal";
	case SpatialInterfaceType::Fastener: return "Fastener";
	case SpatialInterfaceType::Anchor: return "Anchor";
	case SpatialInterfaceType::Hinge: return "Hinge";
	case SpatialInterfaceType::Slide: return "Slide";
	case SpatialInterfaceType::PipePort: return "Pipe port";
	case SpatialInterfaceType::DuctPort: return "Duct port";
	case SpatialInterfaceType::ElectricalPort: return "Electrical port";
	case SpatialInterfaceType::DataPort: return "Data port";
	case SpatialInterfaceType::ControlPort: return "Control port";
	case SpatialInterfaceType::DrainPort: return "Drain port";
	case SpatialInterfaceType::ThermalInterface: return "Thermal interface";
	case SpatialInterfaceType::InspectionInterface: return "Inspection interface";
	}
	return "Unknown";
}

const char *interface_state_name(SpatialInterfaceState state)
{
	switch (state) {
	case SpatialInterfaceState::Available: return "Available";
	case SpatialInterfaceState::Compatible: return "Compatible";
	case SpatialInterfaceState::Connected: return "Connected";
	case SpatialInterfaceState::Invalid: return "Invalid";
	}
	return "Unknown";
}

std::string interface_region_name(const SpatialInterfaceRegion &region)
{
	std::ostringstream text;
	switch (region.kind()) {
	case SpatialInterfaceRegionKind::Point:
		return "Point";
	case SpatialInterfaceRegionKind::PlaneRectangle:
		text << "Plane rectangle " << region.primaryExtent() << " x "
		     << region.secondaryExtent();
		return text.str();
	case SpatialInterfaceRegionKind::AxisSegment:
		text << "Axis segment " << region.primaryExtent();
		return text.str();
	case SpatialInterfaceRegionKind::ObjectBoundaryFace:
		return "Object boundary face " + region.boundaryFaceName();
	}
	return "Unknown";
}

const char *connection_type_name(SpatialConnectionType type)
{
	switch (type) {
	case SpatialConnectionType::Contains: return "Contains";
	case SpatialConnectionType::ConnectedTo: return "Connected to";
	case SpatialConnectionType::DrainsTo: return "Drains to";
	case SpatialConnectionType::Powers: return "Powers";
	case SpatialConnectionType::ServedBy: return "Served by";
	case SpatialConnectionType::Monitors: return "Monitors";
	case SpatialConnectionType::SupportedBy: return "Supported by";
	case SpatialConnectionType::SeatedIn: return "Seated in";
	case SpatialConnectionType::InsertedIn: return "Inserted in";
	case SpatialConnectionType::SealedTo: return "Sealed to";
	case SpatialConnectionType::FixedTo: return "Fixed to";
	case SpatialConnectionType::AlignedWith: return "Aligned with";
	}
	return "Unknown";
}

const char *connection_state_name(SpatialConnectionState state)
{
	switch (state) {
	case SpatialConnectionState::Proposed: return "Proposed";
	case SpatialConnectionState::Compatible: return "Compatible";
	case SpatialConnectionState::Positioned: return "Positioned";
	case SpatialConnectionState::Engaged: return "Engaged";
	case SpatialConnectionState::Seated: return "Seated";
	case SpatialConnectionState::Locked: return "Locked";
	case SpatialConnectionState::Verified: return "Verified";
	case SpatialConnectionState::Broken: return "Broken";
	case SpatialConnectionState::Invalid: return "Invalid";
	}
	return "Unknown";
}

const char *collision_mode_name(CollisionPositionMode mode)
{
	switch (mode) {
	case CollisionPositionMode::Touch: return "Touch";
	case CollisionPositionMode::Gap: return "Gap";
	case CollisionPositionMode::Drop: return "Drop";
	case CollisionPositionMode::Seat: return "Seat";
	case CollisionPositionMode::Insert: return "Insert";
	case CollisionPositionMode::Tangent: return "Tangent";
	case CollisionPositionMode::Between: return "Between";
	case CollisionPositionMode::CenterContact: return "Center contact";
	}
	return "Unknown";
}

const char *resolution_algorithm_name(SpatialResolutionAlgorithm algorithm)
{
	switch (algorithm) {
	case SpatialResolutionAlgorithm::AxisAlignedSweep: return "Axis-aligned sweep";
	case SpatialResolutionAlgorithm::OpeningBoundaryFit: return "Opening boundary fit";
	case SpatialResolutionAlgorithm::AuthoredPlacement: return "Authored placement";
	case SpatialResolutionAlgorithm::ValidationOnly: return "Validation only";
	}
	return "Unknown";
}

const char *resolution_status_name(SpatialResolutionStatus status)
{
	switch (status) {
	case SpatialResolutionStatus::Succeeded: return "Succeeded";
	case SpatialResolutionStatus::Failed: return "Failed";
	case SpatialResolutionStatus::Rejected: return "Rejected";
	}
	return "Unknown";
}

const char *collision_layer_name(CollisionLayer layer)
{
	switch (layer) {
	case CollisionLayer::Structure: return "Structure";
	case CollisionLayer::Envelope: return "Envelope";
	case CollisionLayer::Interior: return "Interior";
	case CollisionLayer::Furniture: return "Furniture";
	case CollisionLayer::Plumbing: return "Plumbing";
	case CollisionLayer::Hvac: return "HVAC";
	case CollisionLayer::Electrical: return "Electrical";
	case CollisionLayer::Equipment: return "Equipment";
	case CollisionLayer::Terrain: return "Terrain";
	case CollisionLayer::Temporary: return "Temporary";
	}
	return "Unknown";
}

std::string collision_mask_text(const CollisionLayerMask &mask)
{
	std::ostringstream text;
	bool first = true;
	for (int layer_index = 0; layer_index < 10; ++layer_index) {
		const CollisionLayer layer = static_cast<CollisionLayer>(layer_index);
		if (!mask.contains(layer)) continue;
		if (!first) text << ", ";
		text << collision_layer_name(layer);
		first = false;
	}
	return first ? "None" : text.str();
}

std::string interface_reference_text(const SpatialInterfaceReference &reference)
{
	return reference.objectId().value() + "." + reference.interfaceId().value();
}

std::uint64_t aggregate_evidence_hash(
	const std::vector<SpatialResolutionInspection> &records)
{
	std::uint64_t hash = 1469598103934665603ull;
	for (const SpatialResolutionInspection &record : records) {
		hash ^= record.evidence_hash;
		hash *= 1099511628211ull;
	}
	return records.empty() ? 0u : hash;
}

void append_selection_entries(
	const SpatialBuildingModel &model,
	const SpatialObjectPrimitiveAssociationIndex &associations,
	const SpatialObjectId &object_id,
	int depth,
	std::vector<SpatialObjectSelectionEntry> *entries)
{
	if (entries == nullptr) return;
	const SpatialBuildingObject *object = model.objects().find(object_id);
	if (object == nullptr) return;
	entries->push_back({
		object_id.value(),
		object->identity().objectName(),
		object->identity().objectClass().value(),
		associations.primitivesForObject(object_id).size(),
		depth});
	for (const SpatialObjectId &child_id : model.containmentTree().childrenOf(object_id)) {
		append_selection_entries(model, associations, child_id, depth + 1, entries);
	}
}

} // namespace

SpatialObjectInspection SpatialObjectInspectionService::inspectObject(
	const SpatialBuildingModel &model,
	const SpatialObjectId &object_id) const
{
	return inspectObject(model, nullptr, object_id);
}

SpatialObjectInspection SpatialObjectInspectionService::inspectObject(
	const SpatialBuildingModel &model,
	const SmallModernBuildingModel *building_model,
	const SpatialObjectId &object_id) const
{
	SpatialObjectInspection inspection;
	const SpatialBuildingObject *object = model.objects().find(object_id);
	if (object == nullptr) return inspection;

	inspection.available = true;
	inspection.object_id = object_id.value();
	inspection.object_name = object->identity().objectName();
	inspection.object_class = object->identity().objectClass().value();
	inspection.taxonomy_path = join_taxonomy(object->identity().taxonomyPath());
	const SpatialObjectId *parent_id = model.containmentTree().parentOf(object_id);
	inspection.container_object_id = parent_id != nullptr ? parent_id->value() : "<root>";
	inspection.state = object_state_name(object->state());
	inspection.parent_frame = object->frameState().parentFrame().isRoot()
		? "World"
		: object->frameState().parentFrame().objectId().value();
	inspection.collision_layer = collision_layer_name(object->collisionPolicy().layer());
	inspection.collision_mask = collision_mask_text(object->collisionPolicy().mask());
	inspection.authored_local_transform = matrix_values(
		object->frameState().authoredLocalTransform());
	inspection.resolution_local_transform = matrix_values(
		object->frameState().resolutionLocalTransform());
	inspection.resolved_world_transform = matrix_values(
		object->frameState().resolvedWorldTransform());
	inspection.translation_uncertainty = vector3_values(
		object->frameState().uncertainty().translationStandardDeviation());
	inspection.rotation_uncertainty_degrees = vector3_values(
		object->frameState().uncertainty().rotationStandardDeviationDegrees());
	inspection.boundary_representation_count =
		object->boundaryModel().representations().size();
	inspection.primitive_instance_indices =
		SpatialObjectPrimitiveAssociationIndex::fromModel(model)
			.primitivesForObject(object_id);

	const SpatialObjectProvenance &provenance = object->identity().provenance();
	inspection.source_range = {
		provenance.startLine(),
		provenance.startColumn(),
		provenance.endLine(),
		provenance.endColumn()};

	for (const SpatialInterface &interface : object->interfaces()) {
		inspection.interfaces.push_back({
			interface.interfaceId().value(),
			interface_type_name(interface.type()),
			interface_region_name(interface.region()),
			interface_state_name(interface.state()),
			vector3_values(interface.localFrame().localOrigin()),
			vector3_values(interface.localFrame().localNormal()),
			vector3_values(interface.localFrame().localTangent()),
			interface.clearance().nominal()});
	}

	for (const SpatialConnection &connection : model.connectionGraph().connections()) {
		if (connection.sourceInterface().objectId() != object_id &&
		    connection.targetInterface().objectId() != object_id) {
			continue;
		}
		inspection.connections.push_back({
			connection.connectionId().value(),
			connection_type_name(connection.connectionType()),
			interface_reference_text(connection.sourceInterface()),
			interface_reference_text(connection.targetInterface()),
			connection_state_name(connection.state())});
	}

	for (const auto &constraint : model.constraintGraph().constraints()) {
		const auto *collision_constraint =
			dynamic_cast<const CollisionPositionConstraint *>(constraint.get());
		if (collision_constraint == nullptr ||
		    (collision_constraint->movingObjectId() != object_id &&
		     collision_constraint->targetObjectId() != object_id)) {
			continue;
		}
		inspection.constraints.push_back({
			collision_constraint->constraintId().value(),
			"Collision position",
			collision_constraint->movingObjectId().value() + "." +
				collision_constraint->movingInterfaceId().value(),
			collision_constraint->targetObjectId().value() + "." +
				collision_constraint->targetInterfaceId().value(),
			collision_mode_name(collision_constraint->mode()),
			collision_constraint->priority()});
	}

	for (const SpatialResolutionRecord &record : model.resolutionRecords()) {
		if (record.sourceObjectId() != object_id && record.targetObjectId() != object_id) {
			continue;
		}
		inspection.resolution_records.push_back({
			record.constraintId().value(),
			record.targetObjectId().value(),
			resolution_algorithm_name(record.algorithm()),
			resolution_status_name(record.status()),
				record.broadPhaseStepCount(),
				record.refinementIterationCount(),
				record.collisionQueryCount(),
			record.resultingClearance(),
			record.residualError(),
			vector3_values(record.contactPoint()),
			vector3_values(record.contactNormal()),
			record.evidenceHash()});
	}
	inspection.aggregate_evidence_hash = aggregate_evidence_hash(
		inspection.resolution_records);
	if (building_model != nullptr) {
		BuildingKnowledgeInspectionService().appendKnowledge(
			*building_model, object_id, &inspection);
	}
	return inspection;
}

SpatialObjectInspection SpatialObjectInspectionService::inspectPrimitive(
	const SpatialBuildingModel &model,
	std::size_t primitive_instance_index) const
{
	return inspectPrimitive(model, nullptr, primitive_instance_index);
}

SpatialObjectInspection SpatialObjectInspectionService::inspectPrimitive(
	const SpatialBuildingModel &model,
	const SmallModernBuildingModel *building_model,
	std::size_t primitive_instance_index) const
{
	const SpatialObjectPrimitiveAssociationIndex associations =
		SpatialObjectPrimitiveAssociationIndex::fromModel(model);
	const std::optional<SpatialObjectId> object_id =
		associations.objectForPrimitive(primitive_instance_index);
	return object_id.has_value()
		? inspectObject(model, building_model, *object_id)
		: SpatialObjectInspection();
}

std::vector<SpatialObjectSelectionEntry>
SpatialObjectInspectionService::buildContainmentSelection(
	const SpatialBuildingModel &model) const
{
	std::vector<SpatialObjectSelectionEntry> entries;
	const SpatialObjectPrimitiveAssociationIndex associations =
		SpatialObjectPrimitiveAssociationIndex::fromModel(model);
	append_selection_entries(
		model,
		associations,
		model.containmentTree().rootObjectId(),
		0,
		&entries);
	return entries;
}
