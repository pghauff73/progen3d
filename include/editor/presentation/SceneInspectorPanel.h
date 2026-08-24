#pragma once

#include "editor/model/PreviewStatistics.h"
#include "editor/model/ScenePrimitiveInspection.h"

class SceneInspectorPanel
{
public:
	void draw(const ScenePrimitiveInspection &inspection,
	          const PreviewStatistics &statistics) const;
};
