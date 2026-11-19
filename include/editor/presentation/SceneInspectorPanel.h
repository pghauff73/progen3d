#pragma once

#include "editor/model/PreviewStatistics.h"
#include "editor/model/ScenePrimitiveInspection.h"

#include <functional>

class SceneInspectorPanel
{
public:
	void draw(const ScenePrimitiveInspection &inspection,
	          const PreviewStatistics &statistics,
	          const std::function<void(std::optional<CubeFace>)> &select_face,
	          const std::function<void()> &reveal_source,
	          bool source_is_current) const;
};
