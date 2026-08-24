#pragma once

class SceneOverlayConfiguration
{
public:
	bool connection_overlay_visible = true;
	bool connection_axes_visible = true;
	bool connection_surfaces_visible = true;
	bool connection_features_visible = true;
	bool connection_bounds_visible = true;
	bool connection_legend_visible = true;
	bool axial_profile_overlay_visible = false;
	bool spatial_overlay_visible = false;
	bool spatial_object_frames_visible = true;
	bool spatial_interfaces_visible = true;
	bool spatial_connections_visible = true;
	bool spatial_constraints_visible = true;
	bool spatial_contacts_visible = true;
	bool spatial_clearances_visible = true;
	bool spatial_bounds_visible = true;
	bool building_function_allocations_visible = false;
	bool building_service_flows_visible = false;
	bool building_requirement_status_visible = false;
	bool building_pending_evidence_visible = false;
};
