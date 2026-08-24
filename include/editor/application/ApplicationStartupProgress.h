#pragma once

#include <array>
#include <cstddef>

enum class ApplicationStartupStage {
	PrepareInterface,
	LoadPartCatalog,
	LoadInterfaceArtwork,
	LoadInitialDocument,
	GenerateInitialScene,
	Complete
};

class ApplicationStartupProgress
{
public:
	void beginStage(ApplicationStartupStage stage);
	void completeStage(ApplicationStartupStage stage);

	ApplicationStartupStage currentStage() const;
	float overallProgress(float current_stage_progress) const;
	int completedStageCount() const;
	int stageCount() const;

private:
	static constexpr std::array<float, 5> stage_weights_{{
		0.15f,
		0.08f,
		0.08f,
		0.09f,
		0.60f
	}};

	static std::size_t stageIndex(ApplicationStartupStage stage);

	ApplicationStartupStage current_stage_ = ApplicationStartupStage::PrepareInterface;
	std::size_t completed_stage_count_ = 0;
};
