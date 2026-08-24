#include "editor/model/GeneratedSceneSnapshot.h"

#include "Context.h"

const SpatialBuildingModel *GeneratedSceneSnapshot::spatialBuildingModel() const
{
	const SceneGenerationContext *context = generationContext();
	return context != nullptr && context->spatialBuildingModel()
		? context->spatialBuildingModel().get()
		: nullptr;
}

const SmallModernBuildingModel *GeneratedSceneSnapshot::smallModernBuildingModel() const
{
	const SceneGenerationContext *context = generationContext();
	return context != nullptr && context->smallModernBuildingModel()
		? context->smallModernBuildingModel().get()
		: nullptr;
}
