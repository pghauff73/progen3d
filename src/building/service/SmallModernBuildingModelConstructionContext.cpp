#include "building/service/SmallModernBuildingModelConstructionContext.h"

#include "building/service/SmallModernBuildingModelConstructionService.h"

SmallModernBuildingModelConstructionResult
SmallModernBuildingModelConstructionContext::finalize(
	const SpatialBuildingModel &spatial_model) const
{
	return SmallModernBuildingModelConstructionService().construct(
		spatial_model, request_);
}
