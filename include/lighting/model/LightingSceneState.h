#pragma once

#include "electrical/model/ElectricalControlGraph.h"
#include "lighting/model/LightId.h"
#include "lighting/model/LightingEvidenceRecord.h"
#include "lighting/model/PreviewLightCollection.h"

#include <string>

class LightingSceneState
{
public:
	LightingSceneState()
	{
		resetToStudio();
	}

	void resetToStudio()
	{
		lights.loadStudioDefault();
		electrical_control_graph.clear();
		evidence = LightingEvidenceRecord{};
		selected_light_id = LightId("StudioKey");
		hovered_light_id = LightId();
		light_gizmos_visible = true;
		activate_controls_mode = false;
		active_preset_name = "Studio";
	}

	PreviewLightCollection lights;
	ElectricalControlGraph electrical_control_graph;
	LightingEvidenceRecord evidence;
	LightId selected_light_id;
	LightId hovered_light_id;
	bool light_gizmos_visible = true;
	bool activate_controls_mode = false;
	std::string active_preset_name;
};
