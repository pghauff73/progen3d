#include "spatial/service/SpatialBuildingModelConstructionService.h"

#include "spatial/graph/SpatialConnectionGraph.h"
#include "spatial/graph/SpatialConstraintGraph.h"
#include "spatial/graph/SpatialContainmentTree.h"
#include "spatial/graph/SpatialObjectRegistry.h"
#include "spatial/model/AxisAlignedBoundingBoundary.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/service/SpatialConstraintDependencyAnalyzer.h"
#include "spatial/service/SpatialContainmentValidationService.h"
#include "spatial/service/SpatialFrameValidationService.h"
#include "spatial/service/SpatialInterfaceCompatibilityService.h"
#include "spatial/service/SpatialResolutionEvidenceHashService.h"
#include "spatial/service/SpatialWorldFrameResolutionService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>

namespace {

constexpr float kMatrixEpsilon = 0.000001f;
constexpr float kConnectionDimensionalTolerance = 0.0005f;

bool is_safe_identifier(const std::string &value)
{
	if (value.empty()) return false;
	const auto is_start = [](char character) {
		return (character >= 'A' && character <= 'Z') ||
		       (character >= 'a' && character <= 'z') || character == '_';
	};
	const auto is_body = [&](char character) {
		return is_start(character) || (character >= '0' && character <= '9') ||
		       character == '.' || character == '-';
	};
	if (!is_start(value.front())) return false;
	return std::all_of(value.begin() + 1, value.end(), is_body);
}

bool is_finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool is_finite(const glm::mat4 &value)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(value[column][row])) return false;
		}
	}
	return true;
}

bool is_invertible(const glm::mat4 &value)
{
	return std::fabs(glm::determinant(glm::mat3(value))) > kMatrixEpsilon;
}

bool clearance_is_valid(const SpatialClearanceRequirement &clearance)
{
	return std::isfinite(clearance.minimum()) &&
	       std::isfinite(clearance.nominal()) &&
	       std::isfinite(clearance.maximum()) &&
	       clearance.minimum() >= 0.0f &&
	       clearance.minimum() <= clearance.nominal() &&
	       clearance.nominal() <= clearance.maximum();
}

bool optional_dimension_is_valid(const std::optional<float> &dimension)
{
	return !dimension.has_value() ||
	       (std::isfinite(*dimension) && *dimension > 0.0f);
}

bool interface_region_is_valid(const SpatialInterfaceRegion &region)
{
	switch (region.kind()) {
	case SpatialInterfaceRegionKind::Point:
		return true;
	case SpatialInterfaceRegionKind::PlaneRectangle:
		return std::isfinite(region.primaryExtent()) &&
		       std::isfinite(region.secondaryExtent()) &&
		       region.primaryExtent() > 0.0f && region.secondaryExtent() > 0.0f;
	case SpatialInterfaceRegionKind::AxisSegment:
		return std::isfinite(region.primaryExtent()) && region.primaryExtent() > 0.0f;
	case SpatialInterfaceRegionKind::ObjectBoundaryFace:
		return !region.boundaryFaceName().empty();
	}
	return false;
}

bool compatibility_profile_is_valid(const InterfaceCompatibilityProfile &profile)
{
	return optional_dimension_is_valid(profile.nominalDiameter()) &&
	       optional_dimension_is_valid(profile.nominalWidth()) &&
	       optional_dimension_is_valid(profile.nominalHeight());
}

const SpatialBuildingObject *find_object(
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialObjectId &object_id)
{
	for (const SpatialBuildingObject &object : objects) {
		if (object.identity().objectId() == object_id) return &object;
	}
	return nullptr;
}

const SpatialInterface *find_interface(
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialInterfaceReference &reference)
{
	const SpatialBuildingObject *object = find_object(objects, reference.objectId());
	return object == nullptr ? nullptr : object->findInterface(reference.interfaceId());
}

void validate_identity(const SpatialBuildingObject &object,
	                   SpatialModelValidationReport *report)
{
	const SpatialObjectIdentity &identity = object.identity();
	if (!is_safe_identifier(identity.objectId().value()) ||
	    identity.objectName().empty() || identity.objectClass().empty() ||
	    identity.taxonomyPath().empty()) {
		report->addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::InvalidIdentity,
			"Spatial object identity '" + identity.objectId().value() +
				"' requires a safe ID, name, class, and taxonomy path.",
			{identity.objectId()}));
	}
	for (const std::string &segment : identity.taxonomyPath().segments()) {
		if (segment.empty()) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidIdentity,
				"Spatial object '" + identity.objectId().value() +
					"' contains an empty taxonomy segment.",
				{identity.objectId()}));
			break;
		}
	}
}

void validate_boundary(const SpatialBuildingObject &object,
	                   SpatialModelValidationReport *report)
{
	for (const auto &representation : object.boundaryModel().representations()) {
		if (!representation) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidBoundary,
				"Spatial object '" + object.identity().objectId().value() +
					"' contains a null boundary representation.",
				{object.identity().objectId()}));
			continue;
		}
		if (representation->kind() != BoundaryRepresentationKind::AxisAlignedBounding) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidBoundary,
				"Spatial object '" + object.identity().objectId().value() +
					"' uses a boundary representation not supported by SMB-OMv2 P0.",
				{object.identity().objectId()}));
			continue;
		}
		const auto *axis_aligned =
			static_cast<const AxisAlignedBoundingBoundary *>(representation.get());
		const AxisAlignedBounds &bounds = axis_aligned->bounds();
		if (!bounds.valid || !is_finite(bounds.min) || !is_finite(bounds.max) ||
		    !is_finite(bounds.center) || !is_finite(bounds.half_extents) ||
		    bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y ||
		    bounds.min.z > bounds.max.z ||
		    bounds.half_extents.x < 0.0f || bounds.half_extents.y < 0.0f ||
		    bounds.half_extents.z < 0.0f) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidBoundary,
				"Spatial object '" + object.identity().objectId().value() +
					"' has an invalid axis-aligned boundary.",
				{object.identity().objectId()}));
		}
	}
}

std::vector<SpatialBuildingObject> normalize_and_validate_objects(
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialModelSafetyLimits &limits,
	SpatialModelValidationReport *report)
{
	SpatialFrameValidationService frame_validation;
	std::set<SpatialObjectId> object_ids;
	std::vector<SpatialBuildingObject> normalized_objects;
	normalized_objects.reserve(objects.size());
	if (objects.size() > limits.maximum_objects) {
		report->addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::SafetyCeilingExceeded,
			"Spatial object count exceeds the configured ceiling."));
	}

	for (const SpatialBuildingObject &object : objects) {
		validate_identity(object, report);
		if (!object_ids.insert(object.identity().objectId()).second) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::DuplicateObjectId,
				"Duplicate spatial object ID '" +
					object.identity().objectId().value() + "'.",
				{object.identity().objectId()}));
		}
		report->append(frame_validation.validateObjectFrame(object));
		validate_boundary(object, report);

		if (object.interfaces().size() > limits.maximum_interfaces_per_object) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::SafetyCeilingExceeded,
				"Spatial object '" + object.identity().objectId().value() +
					"' exceeds the interface ceiling.",
				{object.identity().objectId()}));
		}
		std::set<SpatialInterfaceId> interface_ids;
		std::vector<SpatialInterface> normalized_interfaces;
		normalized_interfaces.reserve(object.interfaces().size());
		for (const SpatialInterface &interface : object.interfaces()) {
			if (!is_safe_identifier(interface.interfaceId().value())) {
				report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::InvalidInterface,
					"Spatial object '" + object.identity().objectId().value() +
						"' has an interface with an invalid ID.",
					{object.identity().objectId()}));
			}
			if (!interface_ids.insert(interface.interfaceId()).second) {
				report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::DuplicateInterfaceId,
					"Duplicate interface ID '" + interface.interfaceId().value() +
						"' on spatial object '" + object.identity().objectId().value() + "'.",
					{object.identity().objectId()}));
			}
			if (interface.ownerObjectId() != object.identity().objectId()) {
				report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::InvalidInterfaceOwner,
					"Interface '" + interface.interfaceId().value() +
						"' does not belong to spatial object '" +
						object.identity().objectId().value() + "'.",
					{object.identity().objectId(), interface.ownerObjectId()}));
			}
			if (!interface_region_is_valid(interface.region()) ||
			    !compatibility_profile_is_valid(interface.compatibility()) ||
			    !clearance_is_valid(interface.clearance())) {
				report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::InvalidInterface,
					"Interface '" + interface.interfaceId().value() +
						"' has invalid region, compatibility, or clearance values.",
					{object.identity().objectId()}));
			}
			SpatialInterfaceFrame normalized_frame(
				glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			std::string diagnostic;
			if (!frame_validation.normalizeInterfaceFrame(
					interface.localFrame(), &normalized_frame, &diagnostic)) {
				report->addIssue(SpatialModelValidationIssue(
					SpatialModelValidationCode::InvalidInterface,
					"Interface '" + interface.interfaceId().value() + "' on object '" +
						object.identity().objectId().value() + "' is invalid: " + diagnostic,
					{object.identity().objectId()}));
				normalized_interfaces.push_back(interface);
			} else {
				normalized_interfaces.push_back(interface.withLocalFrame(normalized_frame));
			}
		}
		normalized_objects.push_back(object.withInterfaces(std::move(normalized_interfaces)));
	}
	return normalized_objects;
}

void validate_connections(const std::vector<SpatialConnection> &connections,
	                      const std::vector<SpatialBuildingObject> &objects,
	                      const SpatialModelSafetyLimits &limits,
	                      SpatialModelValidationReport *report)
{
	if (connections.size() > limits.maximum_connections) {
		report->addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::SafetyCeilingExceeded,
			"Spatial connection count exceeds the configured ceiling."));
	}
	std::set<SpatialConnectionId> connection_ids;
	SpatialInterfaceCompatibilityService compatibility_service;
	for (const SpatialConnection &connection : connections) {
		if (!is_safe_identifier(connection.connectionId().value()) ||
		    !connection_ids.insert(connection.connectionId()).second) {
			report->addIssue(SpatialModelValidationIssue(
				connection.connectionId().empty()
					? SpatialModelValidationCode::InvalidConnection
					: SpatialModelValidationCode::DuplicateConnectionId,
				"Spatial connection ID '" + connection.connectionId().value() +
					"' is invalid or duplicated."));
		}
		const SpatialInterface *source = find_interface(objects, connection.sourceInterface());
		const SpatialInterface *target = find_interface(objects, connection.targetInterface());
		if (source == nullptr || target == nullptr) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UndefinedConnectionEndpoint,
				"Spatial connection '" + connection.connectionId().value() +
					"' references an undefined object or interface.",
				{connection.sourceInterface().objectId(),
				 connection.targetInterface().objectId()}));
			continue;
		}
		if (!clearance_is_valid(connection.clearance()) ||
		    !std::isfinite(connection.insertionDepth()) ||
		    connection.insertionDepth() < 0.0f) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConnection,
				"Spatial connection '" + connection.connectionId().value() +
					"' has invalid clearance or insertion depth."));
		}
		const SpatialInterfaceCompatibilityResult compatibility =
			compatibility_service.evaluate(*source, *target, kConnectionDimensionalTolerance);
		if (!compatibility.isCompatible()) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::IncompatibleInterfaces,
				"Spatial connection '" + connection.connectionId().value() +
					"' is incompatible: " + compatibility.diagnostic(),
				{source->ownerObjectId(), target->ownerObjectId()}));
		}
	}
}

void validate_constraints(
	const std::vector<std::shared_ptr<const SpatialConstraint>> &constraints,
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialModelSafetyLimits &limits,
	SpatialModelValidationReport *report)
{
	if (constraints.size() > limits.maximum_constraints) {
		report->addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::SafetyCeilingExceeded,
			"Spatial constraint count exceeds the configured ceiling."));
	}
	std::set<SpatialConstraintId> constraint_ids;
	SpatialInterfaceCompatibilityService compatibility_service;
	for (const auto &constraint : constraints) {
		if (!constraint) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial constraint collection contains a null constraint."));
			continue;
		}
		if (!is_safe_identifier(constraint->constraintId().value()) ||
		    !constraint_ids.insert(constraint->constraintId()).second) {
			report->addIssue(SpatialModelValidationIssue(
				constraint->constraintId().empty()
					? SpatialModelValidationCode::InvalidConstraint
					: SpatialModelValidationCode::DuplicateConstraintId,
				"Spatial constraint ID '" + constraint->constraintId().value() +
					"' is invalid or duplicated."));
		}
		if (constraint->kind() != SpatialConstraintKind::CollisionPosition) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UnsupportedConstraint,
				"Spatial constraint '" + constraint->constraintId().value() +
					"' is not supported by SMB-OMv2 P0."));
			continue;
		}
		const auto *positioning = dynamic_cast<const CollisionPositionConstraint *>(constraint.get());
		if (positioning == nullptr) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial collision-position constraint has an invalid runtime type."));
			continue;
		}
		const SpatialBuildingObject *moving = find_object(objects, positioning->movingObjectId());
		const SpatialBuildingObject *target = find_object(objects, positioning->targetObjectId());
		const SpatialInterfaceReference moving_reference(
			positioning->movingObjectId(), positioning->movingInterfaceId());
		const SpatialInterfaceReference target_reference(
			positioning->targetObjectId(), positioning->targetInterfaceId());
		const SpatialInterface *moving_interface = find_interface(objects, moving_reference);
		const SpatialInterface *target_interface = find_interface(objects, target_reference);
		if (moving == nullptr || target == nullptr || moving_interface == nullptr ||
		    target_interface == nullptr) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UndefinedConstraintEndpoint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' references an undefined object or interface.",
				{positioning->targetObjectId(), positioning->movingObjectId()}));
			continue;
		}
		if (positioning->movingObjectId() == positioning->targetObjectId()) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial collision-position constraint cannot move an object against itself.",
				{positioning->movingObjectId()}));
		}
		const glm::vec3 direction = positioning->direction().vector();
		const int direction_frame = static_cast<int>(positioning->direction().frame());
		if (!is_finite(direction) || glm::length(direction) <= kMatrixEpsilon ||
		    direction_frame < static_cast<int>(SpatialDirectionFrame::World) ||
		    direction_frame > static_cast<int>(SpatialDirectionFrame::TargetInterface)) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' has an invalid or zero direction."));
		}
		if (positioning->mode() != CollisionPositionMode::Touch &&
		    positioning->mode() != CollisionPositionMode::Gap &&
		    positioning->mode() != CollisionPositionMode::Drop) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::UnsupportedConstraint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' requests a positioning mode not supported by SMB-OMv2 P0."));
		}
		if (!clearance_is_valid(positioning->clearance()) ||
		    !std::isfinite(positioning->seatingDepth()) ||
		    positioning->seatingDepth() != 0.0f ||
		    !std::isfinite(positioning->maximumDistance()) ||
		    positioning->maximumDistance() <= 0.0f ||
		    !std::isfinite(positioning->tolerance()) ||
		    positioning->tolerance() < 0.000001f) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' has invalid clearance, travel, seating depth, or tolerance."));
		}
		if (positioning->collisionMask().bits() == 0u ||
		    !positioning->collisionMask().contains(target->collisionPolicy().layer()) ||
		    !moving->collisionPolicy().blocks(target->collisionPolicy())) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidConstraint,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' collision masks do not permit target contact.",
				{positioning->targetObjectId(), positioning->movingObjectId()}));
		}
		const SpatialInterfaceCompatibilityResult compatibility =
			compatibility_service.evaluate(
				*moving_interface, *target_interface, positioning->tolerance());
		if (!compatibility.isCompatible()) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::IncompatibleInterfaces,
				"Spatial constraint '" + positioning->constraintId().value() +
					"' has incompatible interfaces: " + compatibility.diagnostic(),
				{positioning->targetObjectId(), positioning->movingObjectId()}));
		}
	}
}

void validate_bindings(const SpatialBuildingModelConstructionRequest &request,
	                   const std::vector<SpatialBuildingObject> &objects,
	                   const SpatialModelSafetyLimits &limits,
	                   SpatialModelValidationReport *report)
{
	std::map<SpatialObjectId, std::size_t> binding_counts;
	std::set<std::size_t> bound_primitive_indices;
	for (const SpatialObjectGeometryBinding &binding : request.geometryBindings()) {
		++binding_counts[binding.objectId()];
		if (find_object(objects, binding.objectId()) == nullptr ||
		    !is_finite(binding.primitiveLocalToObjectTransform()) ||
		    !is_invertible(binding.primitiveLocalToObjectTransform()) ||
		    (request.primitiveInstanceCount().has_value() &&
		     binding.primitiveInstanceIndex() >= *request.primitiveInstanceCount())) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidGeometryBinding,
				"Spatial geometry binding for object '" + binding.objectId().value() +
					"' is invalid.",
				{binding.objectId()}));
		}
		if (!bound_primitive_indices.insert(binding.primitiveInstanceIndex()).second) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::DuplicatePrimitiveBinding,
				"Scene primitive index " +
					std::to_string(binding.primitiveInstanceIndex()) +
					" is bound to more than one spatial object."));
		}
	}
	for (const auto &entry : binding_counts) {
		if (entry.second > limits.maximum_primitive_bindings_per_object) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::SafetyCeilingExceeded,
				"Spatial object '" + entry.first.value() +
					"' exceeds the primitive-binding ceiling.",
				{entry.first}));
		}
	}
}

void validate_resolution_records(
	const SpatialBuildingModelConstructionRequest &request,
	const std::vector<SpatialBuildingObject> &objects,
	const SpatialModelSafetyLimits &limits,
	SpatialModelValidationReport *report)
{
	std::map<SpatialObjectId, std::size_t> record_counts;
	std::set<SpatialConstraintId> constraint_ids;
	for (const auto &constraint : request.constraints()) {
		if (constraint) constraint_ids.insert(constraint->constraintId());
	}
	SpatialResolutionEvidenceHashService hash_service;
	for (const SpatialResolutionRecord &record : request.resolutionRecords()) {
		++record_counts[record.sourceObjectId()];
		const bool finite_values =
			is_finite(record.initialWorldTransform()) &&
			is_finite(record.finalWorldTransform()) &&
			is_finite(record.contactPoint()) && is_finite(record.contactNormal()) &&
			std::isfinite(record.tolerance()) &&
			std::isfinite(record.resultingClearance()) &&
			std::isfinite(record.residualError());
		if (find_object(objects, record.sourceObjectId()) == nullptr ||
		    find_object(objects, record.targetObjectId()) == nullptr ||
		    constraint_ids.find(record.constraintId()) == constraint_ids.end() ||
		    !finite_values || record.broadPhaseStepCount() < 0 ||
		    record.refinementIterationCount() < 0 || record.tolerance() <= 0.0f ||
		    record.status() != SpatialResolutionStatus::Succeeded ||
		    record.evidenceHash() == 0u ||
		    record.evidenceHash() != hash_service.calculate(record)) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::InvalidResolutionRecord,
				"Spatial resolution record for constraint '" +
					record.constraintId().value() + "' is invalid or has a stale evidence hash.",
				{record.sourceObjectId(), record.targetObjectId()}));
		}
	}
	for (const auto &entry : record_counts) {
		if (entry.second > limits.maximum_resolution_records_per_object) {
			report->addIssue(SpatialModelValidationIssue(
				SpatialModelValidationCode::SafetyCeilingExceeded,
				"Spatial object '" + entry.first.value() +
					"' exceeds the resolution-record ceiling.",
				{entry.first}));
		}
	}
}

} // namespace

SpatialBuildingModelConstructionResult SpatialBuildingModelConstructionService::construct(
	const SpatialBuildingModelConstructionRequest &request,
	const SpatialModelSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	std::vector<SpatialBuildingObject> normalized_objects =
		normalize_and_validate_objects(request.objects(), limits, &report);

	SpatialContainmentValidationService containment_validation;
	report.append(containment_validation.validate(
		normalized_objects,
		request.rootObjectId(),
		request.containmentRelationships(),
		limits));
	validate_connections(request.connections(), normalized_objects, limits, &report);
	validate_constraints(request.constraints(), normalized_objects, limits, &report);
	validate_bindings(request, normalized_objects, limits, &report);
	validate_resolution_records(request, normalized_objects, limits, &report);

	if (!report.isValid()) {
		return SpatialBuildingModelConstructionResult(nullptr, std::move(report));
	}

	SpatialObjectRegistry object_registry(std::move(normalized_objects));
	SpatialContainmentTree containment_tree(
		request.rootObjectId(), request.containmentRelationships());
	SpatialConstraintGraph constraint_graph(request.constraints());
	SpatialConstraintDependencyAnalyzer dependency_analyzer;
	const SpatialConstraintDependencyReport dependency_report =
		dependency_analyzer.analyze(constraint_graph, object_registry, limits);
	report.append(dependency_report.validationReport());
	if (!dependency_report.isAcyclic()) {
		return SpatialBuildingModelConstructionResult(nullptr, std::move(report));
	}

	SpatialWorldFrameResolutionService world_frame_resolution;
	const std::optional<SpatialObjectRegistry> resolved_registry =
		world_frame_resolution.resolve(object_registry, containment_tree, &report);
	if (!resolved_registry.has_value() || !report.isValid()) {
		return SpatialBuildingModelConstructionResult(nullptr, std::move(report));
	}

	auto model = std::make_shared<const SpatialBuildingModel>(
		*resolved_registry,
		std::move(containment_tree),
		SpatialConnectionGraph(request.connections()),
		std::move(constraint_graph),
		request.geometryBindings(),
		request.resolutionRecords());
	return SpatialBuildingModelConstructionResult(std::move(model), std::move(report));
}
