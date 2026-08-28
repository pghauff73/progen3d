#pragma once

#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialModelValidationReport.h"

#include <memory>
#include <utility>

class SpatialBuildingModelConstructionResult {
public:
	SpatialBuildingModelConstructionResult(
		std::shared_ptr<const SpatialBuildingModel> model,
		SpatialModelValidationReport validation_report)
		: model_(std::move(model)),
		  validation_report_(std::move(validation_report)) {}

	bool succeeded() const { return model_ != nullptr && validation_report_.isValid(); }
	const std::shared_ptr<const SpatialBuildingModel> &model() const { return model_; }
	const SpatialModelValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	std::shared_ptr<const SpatialBuildingModel> model_;
	SpatialModelValidationReport validation_report_;
};
