#include "lighting/model/LightingSceneDefinition.h"

bool LightingSceneDefinition::addLight(SceneLight light, std::string *diagnostic)
{
	if (light.id().empty()) {
		if (diagnostic != nullptr) *diagnostic = "Light requires a non-empty stable ID.";
		return false;
	}
	for (const SceneLight &existing : lights_) {
		if (existing.id() == light.id()) {
			if (diagnostic != nullptr) {
				*diagnostic = "Light ID '" + light.id().value() + "' is declared more than once.";
			}
			return false;
		}
	}
	lights_.push_back(std::move(light));
	return true;
}

bool LightingSceneDefinition::addFixture(
	LightFixtureObject fixture,
	std::string *diagnostic)
{
	if (electrical_control_graph_.addFixture(std::move(fixture))) return true;
	if (diagnostic != nullptr) *diagnostic = "LightFixture requires a unique non-empty ID.";
	return false;
}

bool LightingSceneDefinition::addFixtureWithEmitter(
	SceneLight light,
	LightFixtureObject fixture,
	std::string *diagnostic)
{
	LightingSceneDefinition candidate = *this;
	if (!candidate.addLight(std::move(light), diagnostic) ||
	    !candidate.addFixture(std::move(fixture), diagnostic)) {
		return false;
	}
	*this = std::move(candidate);
	return true;
}

bool LightingSceneDefinition::addSwitch(LightSwitch light_switch, std::string *diagnostic)
{
	if (electrical_control_graph_.addSwitch(std::move(light_switch))) return true;
	if (diagnostic != nullptr) *diagnostic = "LightSwitch requires a unique non-empty ID.";
	return false;
}

bool LightingSceneDefinition::addCircuit(LightingCircuit circuit, std::string *diagnostic)
{
	if (electrical_control_graph_.addCircuit(std::move(circuit))) return true;
	if (diagnostic != nullptr) *diagnostic = "LightingCircuit requires a unique non-empty ID.";
	return false;
}
