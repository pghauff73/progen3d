#pragma once

#include "lighting/model/SceneLight.h"

#include <cstddef>
#include <vector>

class PreviewLightCollection
{
public:
	static constexpr std::size_t maximum_visible_lights = 32;

	LightId add(SceneLight light);
	bool remove(const LightId &id);
	SceneLight *find(const LightId &id);
	const SceneLight *find(const LightId &id) const;

	std::vector<SceneLight> &lights() { return lights_; }
	const std::vector<SceneLight> &lights() const { return lights_; }

	void clear();
	void loadStudioDefault();
	void replaceGrammarLights(const std::vector<SceneLight> &grammar_lights);

private:
	LightId uniqueIdFor(const LightId &requested_id) const;

	std::vector<SceneLight> lights_;
};
