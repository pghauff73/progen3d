#pragma once

#include "editor/model/GeneratedSceneSnapshot.h"
#include "grammar.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class SceneGenerationResult
{
public:
	std::unique_ptr<GeneratedSceneSnapshot> scene_snapshot;
	std::vector<int> diagnostic_lines;
	std::vector<GrammarDiagnostic> diagnostics;
	std::uint64_t request_id = 0;
	std::uint64_t design_nonce = 0;
	double evaluation_time = 0.0;
	double generation_milliseconds = 0.0;
	bool time_sample = false;
	std::string error;

	bool succeeded() const
	{
		return scene_snapshot != nullptr && error.empty() && diagnostics.empty();
	}
};
