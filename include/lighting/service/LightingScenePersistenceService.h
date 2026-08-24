#pragma once

#include "lighting/model/LightingSceneState.h"

#include <filesystem>
#include <string>

class LightingScenePersistenceService
{
public:
	std::filesystem::path sidecarPathFor(
		const std::filesystem::path &grammar_path) const;
	bool save(const std::filesystem::path &grammar_path,
	          const LightingSceneState &lighting_state,
	          std::string *error_message) const;
	bool load(const std::filesystem::path &grammar_path,
	          LightingSceneState *lighting_state,
	          std::string *error_message) const;
};
