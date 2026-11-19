#pragma once

#include "geometry/model/CubeFace.h"

#include <optional>
#include <string>

class PreviewPointerPosition
{
public:
	float x = 0.0f;
	float y = 0.0f;
};

class EditorSelection
{
public:
	int hovered_instance_index = -1;
	int selected_instance_index = -1;
	std::optional<CubeFace> hovered_face;
	std::optional<CubeFace> selected_face;
	std::string selected_spatial_object_id;
	bool pointer_selection_armed = false;
	PreviewPointerPosition pointer_selection_start;
};
