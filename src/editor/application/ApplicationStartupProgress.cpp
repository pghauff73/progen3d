#include "editor/application/ApplicationStartupProgress.h"

#include <algorithm>

void ApplicationStartupProgress::beginStage(ApplicationStartupStage stage)
{
	current_stage_ = stage;
}

void ApplicationStartupProgress::completeStage(ApplicationStartupStage stage)
{
	const std::size_t completed_count = stageIndex(stage) + 1;
	completed_stage_count_ = std::max(completed_stage_count_, completed_count);
	current_stage_ = completed_stage_count_ >= stage_weights_.size()
		                 ? ApplicationStartupStage::Complete
		                 : static_cast<ApplicationStartupStage>(completed_stage_count_);
}

ApplicationStartupStage ApplicationStartupProgress::currentStage() const
{
	return current_stage_;
}

float ApplicationStartupProgress::overallProgress(float current_stage_progress) const
{
	float progress = 0.0f;
	for (std::size_t index = 0;
	     index < completed_stage_count_ && index < stage_weights_.size();
	     ++index) {
		progress += stage_weights_[index];
	}

	if (completed_stage_count_ < stage_weights_.size()) {
		progress += stage_weights_[completed_stage_count_] *
		            std::clamp(current_stage_progress, 0.0f, 1.0f);
	}
	return std::clamp(progress, 0.0f, 1.0f);
}

int ApplicationStartupProgress::completedStageCount() const
{
	return static_cast<int>(completed_stage_count_);
}

int ApplicationStartupProgress::stageCount() const
{
	return static_cast<int>(stage_weights_.size());
}

std::size_t ApplicationStartupProgress::stageIndex(ApplicationStartupStage stage)
{
	const std::size_t index = static_cast<std::size_t>(stage);
	return std::min(index, stage_weights_.size() - 1);
}
