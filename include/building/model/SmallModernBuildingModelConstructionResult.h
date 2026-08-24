#pragma once

#include "building/model/BuildingModelValidationReport.h"
#include "building/model/SmallModernBuildingModelConstructionMetrics.h"

#include <memory>
#include <utility>

class SmallModernBuildingModel;

class SmallModernBuildingModelConstructionResult {
public:
	SmallModernBuildingModelConstructionResult(
		std::shared_ptr<const SmallModernBuildingModel> model,
		BuildingModelValidationReport validation_report,
		SmallModernBuildingModelConstructionMetrics metrics)
		: model_(std::move(model)),
		  validation_report_(std::move(validation_report)),
		  metrics_(std::move(metrics)) {}

	bool succeeded() const { return model_ != nullptr && validation_report_.isValid(); }
	const std::shared_ptr<const SmallModernBuildingModel> &model() const { return model_; }
	const BuildingModelValidationReport &validationReport() const
	{
		return validation_report_;
	}
	const SmallModernBuildingModelConstructionMetrics &metrics() const { return metrics_; }

private:
	std::shared_ptr<const SmallModernBuildingModel> model_;
	BuildingModelValidationReport validation_report_;
	SmallModernBuildingModelConstructionMetrics metrics_;
};
