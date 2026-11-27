#pragma once

inline constexpr int kSceneSimulationTicksPerSecond = 120;
inline constexpr float kSceneSimulationStepSeconds =
	1.0f / static_cast<float>(kSceneSimulationTicksPerSecond);
