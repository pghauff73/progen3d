#include "editor/service/SpatialOverlayEvidenceService.h"

#include "building/model/SmallModernBuildingModel.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/service/SpatialDirectionResolutionService.h"
#include "spatial/service/SpatialInterfaceQueryService.h"

#include <algorithm>
#include <cmath>

namespace {

void append_segment(std::vector<SpatialOverlaySegment> *segments,
	                const glm::vec3 &start,
	                const glm::vec3 &end,
	                SpatialOverlayColorRole role)
{
	if (segments == nullptr || start == end) return;
	segments->emplace_back(start, end, role);
}

void append_bounds(std::vector<SpatialOverlaySegment> *segments,
	               const AxisAlignedBounds &bounds,
	               SpatialOverlayColorRole role)
{
	if (segments == nullptr || !bounds.valid) return;
	const glm::vec3 corners[8] = {
		{bounds.min.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.max.y, bounds.max.z},
		{bounds.min.x, bounds.max.y, bounds.max.z}};
	const int edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}};
	for (const auto &edge : edges) {
		append_segment(segments, corners[edge[0]], corners[edge[1]], role);
	}
}

SpatialOverlayColorRole state_role(SpatialObjectState state)
{
	if (state == SpatialObjectState::Invalid) return SpatialOverlayColorRole::Invalid;
	if (state == SpatialObjectState::Validated ||
	    state == SpatialObjectState::Constrained ||
	    state == SpatialObjectState::Connected ||
	    state == SpatialObjectState::ContactResolved) {
		return SpatialOverlayColorRole::Validated;
	}
	return SpatialOverlayColorRole::Pending;
}

const AxisAlignedBounds &bounds_for(
	const SpatialOverlayEvidenceRequest &request,
	const SpatialObjectId &object_id)
{
	static const AxisAlignedBounds empty_bounds;
	const auto found = request.object_world_bounds.find(object_id);
	return found != request.object_world_bounds.end() ? found->second : empty_bounds;
}

struct InterfaceEvidence {
	const SpatialBuildingObject *object = nullptr;
	const SpatialInterface *interface = nullptr;
	SpatialInterfaceProjection projection{
		SpatialInterfaceWorldFrame(glm::vec3(0.0f), {0.0f, 1.0f, 0.0f},
		                           {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
		{}};
};

InterfaceEvidence resolve_interface(
	const SpatialBuildingModel &model,
	const SpatialInterfaceReference &reference,
	const SpatialOverlayEvidenceRequest &request)
{
	InterfaceEvidence evidence;
	evidence.object = model.objects().find(reference.objectId());
	if (evidence.object == nullptr) return evidence;
	evidence.interface = evidence.object->findInterface(reference.interfaceId());
	if (evidence.interface == nullptr) return evidence;
	evidence.projection = SpatialInterfaceQueryService().project(
		*evidence.object,
		*evidence.interface,
		evidence.object->frameState().resolvedWorldTransform(),
		bounds_for(request, reference.objectId()));
	return evidence;
}

void append_interface_frame(
	std::vector<SpatialOverlaySegment> *segments,
	const SpatialInterfaceProjection &projection,
	float marker_length)
{
	const SpatialInterfaceWorldFrame &frame = projection.worldFrame();
	const float normal_length = marker_length;
	const float tangent_length = marker_length * 0.72f;
	append_segment(segments, frame.origin(), frame.origin() + frame.normal() * normal_length,
	               SpatialOverlayColorRole::Interface);
	append_segment(segments, frame.origin(), frame.origin() + frame.tangent() * tangent_length,
	               SpatialOverlayColorRole::FrameX);
	append_segment(segments, frame.origin(), frame.origin() + frame.bitangent() * tangent_length,
	               SpatialOverlayColorRole::FrameZ);
	append_bounds(segments, projection.worldRegionBounds(), SpatialOverlayColorRole::Interface);
}

void append_contact_marker(
	std::vector<SpatialOverlaySegment> *segments,
	const glm::vec3 &point,
	float marker_length,
	SpatialOverlayColorRole role)
{
	const float half = marker_length * 0.18f;
	append_segment(segments, point - glm::vec3(half, 0.0f, 0.0f),
	               point + glm::vec3(half, 0.0f, 0.0f), role);
	append_segment(segments, point - glm::vec3(0.0f, half, 0.0f),
	               point + glm::vec3(0.0f, half, 0.0f), role);
	append_segment(segments, point - glm::vec3(0.0f, 0.0f, half),
	               point + glm::vec3(0.0f, 0.0f, half), role);
}

glm::vec3 object_center(
	const SpatialBuildingModel &model,
	const SpatialOverlayEvidenceRequest &request,
	const SpatialObjectId &object_id)
{
	const AxisAlignedBounds &bounds = bounds_for(request, object_id);
	if (bounds.valid) return (bounds.min + bounds.max) * 0.5f;
	const SpatialBuildingObject *object = model.objects().find(object_id);
	return object != nullptr
		? glm::vec3(object->frameState().resolvedWorldTransform()[3])
		: glm::vec3(0.0f);
}

bool has_pending_applicability(const BuildingObjectModelApplicability &applicability)
{
	return applicability.geometry() == BuildingModelApplicability::PendingEvidence ||
	       applicability.spatialBoundary() == BuildingModelApplicability::PendingEvidence ||
	       applicability.spatialInterfaces() == BuildingModelApplicability::PendingEvidence ||
	       applicability.functionalRoles() == BuildingModelApplicability::PendingEvidence ||
	       applicability.buildingFunctions() == BuildingModelApplicability::PendingEvidence ||
	       applicability.servicePorts() == BuildingModelApplicability::PendingEvidence ||
	       applicability.requirements() == BuildingModelApplicability::PendingEvidence ||
	       applicability.operationalState() == BuildingModelApplicability::PendingEvidence ||
	       applicability.conditionState() == BuildingModelApplicability::PendingEvidence ||
	       applicability.scenarioParticipation() == BuildingModelApplicability::PendingEvidence;
}

SpatialOverlayColorRole requirement_role(
	const SmallModernBuildingModel &building_model,
	const BuildingObjectSemanticProfile &profile)
{
	bool unresolved = false;
	for (const BuildingRequirementId &requirement_id : profile.requirementIds()) {
		const BuildingRequirementEvaluationRecord *evaluation =
			building_model.requirementModel().findEvaluation(requirement_id);
		if (evaluation == nullptr ||
		    evaluation->status() == BuildingRequirementEvaluationStatus::Unknown ||
		    evaluation->status() == BuildingRequirementEvaluationStatus::NotEvaluated) {
			unresolved = true;
			continue;
		}
		if (evaluation->status() == BuildingRequirementEvaluationStatus::Failed) {
			return SpatialOverlayColorRole::Invalid;
		}
	}
	return unresolved
		? SpatialOverlayColorRole::BuildingPendingEvidence
		: SpatialOverlayColorRole::BuildingRequirement;
}

} // namespace

std::vector<SpatialOverlaySegment> SpatialOverlayEvidenceService::build(
	const SpatialBuildingModel &model,
	const SpatialOverlayEvidenceRequest &request) const
{
	return build(model, nullptr, request);
}

std::vector<SpatialOverlaySegment> SpatialOverlayEvidenceService::build(
	const SpatialBuildingModel &model,
	const SmallModernBuildingModel *building_model,
	const SpatialOverlayEvidenceRequest &request) const
{
	std::vector<SpatialOverlaySegment> segments;
	const float marker_length = std::max(request.marker_length, 0.01f);
	const SpatialObjectId selected_id(request.selected_object_id);
	const SpatialBuildingObject *selected_object =
		request.selected_object_id.empty() ? nullptr : model.objects().find(selected_id);

	if (selected_object != nullptr && request.object_frames_visible) {
		const glm::mat4 &world = selected_object->frameState().resolvedWorldTransform();
		const glm::vec3 origin(world[3]);
		append_segment(&segments, origin,
		               origin + glm::normalize(glm::vec3(world[0])) * marker_length,
		               SpatialOverlayColorRole::FrameX);
		append_segment(&segments, origin,
		               origin + glm::normalize(glm::vec3(world[1])) * marker_length,
		               SpatialOverlayColorRole::FrameY);
		append_segment(&segments, origin,
		               origin + glm::normalize(glm::vec3(world[2])) * marker_length,
		               SpatialOverlayColorRole::FrameZ);
	}
	if (selected_object != nullptr && request.bounds_visible) {
		append_bounds(&segments, bounds_for(request, selected_id),
		              state_role(selected_object->state()));
	}
	if (selected_object != nullptr && request.interfaces_visible) {
		const AxisAlignedBounds &selected_bounds = bounds_for(request, selected_id);
		for (const SpatialInterface &interface : selected_object->interfaces()) {
			append_interface_frame(
				&segments,
				SpatialInterfaceQueryService().project(
					*selected_object,
					interface,
					selected_object->frameState().resolvedWorldTransform(),
					selected_bounds),
				marker_length);
		}
	}

	if (request.connections_visible) {
		for (const SpatialConnection &connection : model.connectionGraph().connections()) {
			const InterfaceEvidence source = resolve_interface(
				model, connection.sourceInterface(), request);
			const InterfaceEvidence target = resolve_interface(
				model, connection.targetInterface(), request);
			if (source.interface == nullptr || target.interface == nullptr) continue;
			append_segment(
				&segments,
				source.projection.worldFrame().origin(),
				target.projection.worldFrame().origin(),
				connection.state() == SpatialConnectionState::Invalid
					? SpatialOverlayColorRole::Invalid
					: SpatialOverlayColorRole::Connection);
		}
	}

	if (request.constraints_visible) {
		for (const auto &constraint : model.constraintGraph().constraints()) {
			const auto *collision =
				dynamic_cast<const CollisionPositionConstraint *>(constraint.get());
			if (collision == nullptr) continue;
			const SpatialBuildingObject *moving = model.objects().find(collision->movingObjectId());
			const SpatialBuildingObject *target = model.objects().find(collision->targetObjectId());
			if (moving == nullptr || target == nullptr) continue;
			const SpatialInterface *moving_interface =
				moving->findInterface(collision->movingInterfaceId());
			const SpatialInterface *target_interface =
				target->findInterface(collision->targetInterfaceId());
			if (moving_interface == nullptr || target_interface == nullptr) continue;
			const SpatialInterfaceWorldFrame moving_frame =
				SpatialInterfaceQueryService().resolveWorldFrame(
					*moving_interface, moving->frameState().resolvedWorldTransform());
			std::string diagnostic;
			const std::optional<glm::vec3> direction =
				SpatialDirectionResolutionService().resolve(
					collision->direction(),
					*moving,
					*moving_interface,
					*target,
					*target_interface,
					model,
					&diagnostic);
			if (direction.has_value()) {
				append_segment(
					&segments,
					moving_frame.origin(),
					moving_frame.origin() + *direction * marker_length * 1.5f,
					SpatialOverlayColorRole::Constraint);
			}
		}
	}

	for (const SpatialResolutionRecord &record : model.resolutionRecords()) {
		const SpatialOverlayColorRole status_role =
			record.status() == SpatialResolutionStatus::Succeeded
				? SpatialOverlayColorRole::Validated
				: SpatialOverlayColorRole::Invalid;
		if (request.contacts_visible) {
			append_contact_marker(&segments, record.contactPoint(), marker_length, status_role);
			if (glm::length(record.contactNormal()) > 0.000001f) {
				append_segment(
					&segments,
					record.contactPoint(),
					record.contactPoint() + glm::normalize(record.contactNormal()) * marker_length,
					status_role);
			}
			append_segment(
				&segments,
				glm::vec3(record.initialWorldTransform()[3]),
				glm::vec3(record.finalWorldTransform()[3]),
				SpatialOverlayColorRole::Constraint);
		}
		if (request.clearances_visible &&
		    std::fabs(record.resultingClearance()) > 0.000001f &&
		    glm::length(record.contactNormal()) > 0.000001f) {
			append_segment(
				&segments,
				record.contactPoint(),
				record.contactPoint() + glm::normalize(record.contactNormal()) *
					record.resultingClearance(),
				SpatialOverlayColorRole::Clearance);
		}
	}

	if (building_model != nullptr && selected_object != nullptr) {
		const BuildingObjectSemanticProfile *profile =
			building_model->classificationModel().findProfile(selected_id);
		if (profile != nullptr && request.building_requirement_status_visible) {
			append_bounds(
				&segments, bounds_for(request, selected_id),
				requirement_role(*building_model, *profile));
		}
		if (profile != nullptr && request.building_function_allocations_visible &&
		    !profile->functionAllocationIds().empty()) {
			append_contact_marker(
				&segments, object_center(model, request, selected_id), marker_length,
				SpatialOverlayColorRole::BuildingFunction);
		}
		if (profile != nullptr && request.building_pending_evidence_visible &&
		    has_pending_applicability(profile->applicability())) {
			append_bounds(
				&segments, bounds_for(request, selected_id),
				SpatialOverlayColorRole::BuildingPendingEvidence);
		}
		if (request.building_service_flows_visible) {
			for (const BuildingServiceFlow &flow : building_model->serviceModel().flows()) {
				const BuildingServicePort *source_port =
					building_model->serviceModel().findPort(flow.sourcePortId());
				const BuildingServicePort *target_port =
					building_model->serviceModel().findPort(flow.targetPortId());
				if (source_port == nullptr || target_port == nullptr ||
				    (source_port->ownerObjectId() != selected_id &&
				     target_port->ownerObjectId() != selected_id) ||
				    source_port->ownerObjectId() == target_port->ownerObjectId()) {
					continue;
				}
				append_segment(
					&segments,
					object_center(model, request, source_port->ownerObjectId()),
					object_center(model, request, target_port->ownerObjectId()),
					SpatialOverlayColorRole::BuildingServiceFlow);
			}
		}
	}

	return segments;
}
