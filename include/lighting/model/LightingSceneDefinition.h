#pragma once

#include "electrical/model/ElectricalControlGraph.h"
#include "lighting/model/SceneLight.h"

#include <string>
#include <vector>

class LightingSceneDefinition
{
public:
	bool addLight(SceneLight light, std::string *diagnostic);
	bool addFixture(LightFixtureObject fixture, std::string *diagnostic);
	bool addFixtureWithEmitter(SceneLight light,
	                           LightFixtureObject fixture,
	                           std::string *diagnostic);
	bool addSwitch(LightSwitch light_switch, std::string *diagnostic);
	bool addCircuit(LightingCircuit circuit, std::string *diagnostic);

	const std::vector<SceneLight> &lights() const { return lights_; }
	const ElectricalControlGraph &electricalControlGraph() const { return electrical_control_graph_; }

private:
	std::vector<SceneLight> lights_;
	ElectricalControlGraph electrical_control_graph_;
};
