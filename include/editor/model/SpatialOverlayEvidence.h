#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/SpatialObjectId.h"

#include <map>
#include <string>

#include <glm/glm.hpp>

enum class SpatialOverlayColorRole {
	Validated,
	Pending,
	Interface,
	Connection,
	Clearance,
	Constraint,
	BuildingFunction,
	BuildingServiceFlow,
	BuildingRequirement,
	BuildingPendingEvidence,
	Invalid,
	FrameX,
	FrameY,
	FrameZ
};

class SpatialOverlaySegment
{
public:
	SpatialOverlaySegment(glm::vec3 start,
	                      glm::vec3 end,
	                      SpatialOverlayColorRole color_role)
		: start_(start), end_(end), color_role_(color_role)
	{
	}

	const glm::vec3 &start() const { return start_; }
	const glm::vec3 &end() const { return end_; }
	SpatialOverlayColorRole colorRole() const { return color_role_; }

private:
	glm::vec3 start_{0.0f};
	glm::vec3 end_{0.0f};
	SpatialOverlayColorRole color_role_ = SpatialOverlayColorRole::Pending;
};

class SpatialOverlayEvidenceRequest
{
public:
	std::string selected_object_id;
	std::map<SpatialObjectId, AxisAlignedBounds> object_world_bounds;
	float marker_length = 0.5f;
	bool object_frames_visible = true;
	bool interfaces_visible = true;
	bool connections_visible = true;
	bool constraints_visible = true;
	bool contacts_visible = true;
	bool clearances_visible = true;
	bool bounds_visible = true;
	bool building_function_allocations_visible = false;
	bool building_service_flows_visible = false;
	bool building_requirement_status_visible = false;
	bool building_pending_evidence_visible = false;
};
