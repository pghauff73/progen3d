#pragma once

#include "editor/model/SpatialObjectInspection.h"

#include <functional>
#include <string>
#include <vector>

class SpatialObjectInspectorPanel
{
public:
	void draw(const SpatialObjectInspection &inspection,
	          const std::vector<SpatialObjectSelectionEntry> &selection_entries,
	          const std::string &selected_object_id,
	          const std::function<void(const std::string &)> &select_object) const;
};
