#pragma once

#include "editor/model/SpatialObjectInspection.h"

#include <string>
#include <vector>

class SpatialObjectInspectorPanel
{
public:
	void draw(const SpatialObjectInspection &inspection,
	          const std::vector<SpatialObjectSelectionEntry> &selection_entries,
	          std::string *selected_object_id,
	          int *selected_primitive_instance_index) const;
};
