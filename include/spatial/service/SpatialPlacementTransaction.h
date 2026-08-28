#pragma once

#include "Context.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialConstraintResolutionResult.h"

#include <string>
#include <vector>

class SpatialPlacementTransaction {
public:
	SpatialPlacementTransaction(
		const SpatialBuildingModel &initial_model,
		const SceneGenerationContext &scene_context);

	const SpatialBuildingModel &candidateModel() const { return candidate_model_; }
	const std::vector<ScenePrimitiveInstance> &candidatePrimitiveInstances() const
	{
		return candidate_primitive_instances_;
	}

	bool stageConstraintResolution(
		const SpatialConstraintResolutionResult &resolution,
		std::string *diagnostic);
	bool validate(std::string *diagnostic) const;
	bool commit(SpatialBuildingModel *model,
	            SceneGenerationContext *scene_context,
	            std::string *diagnostic);
	void discard();

private:
	SpatialBuildingModel candidate_model_;
	std::vector<ScenePrimitiveInstance> candidate_primitive_instances_;
	bool active_ = true;
};
