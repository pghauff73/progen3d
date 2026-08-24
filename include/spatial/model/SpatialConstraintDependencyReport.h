#pragma once

#include "spatial/model/SpatialConstraintId.h"
#include "spatial/model/SpatialModelValidationReport.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>
#include <vector>

class SpatialConstraintDependencyReport {
public:
	SpatialConstraintDependencyReport(
		std::vector<SpatialObjectId> topological_object_order,
		std::vector<SpatialConstraintId> topological_constraint_order,
		std::vector<SpatialObjectId> cycle_path,
		SpatialModelValidationReport validation_report)
		: topological_object_order_(std::move(topological_object_order)),
		  topological_constraint_order_(std::move(topological_constraint_order)),
		  cycle_path_(std::move(cycle_path)),
		  validation_report_(std::move(validation_report)) {}

	bool isAcyclic() const
	{
		return cycle_path_.empty() && validation_report_.isValid();
	}

	const std::vector<SpatialObjectId> &topologicalObjectOrder() const
	{
		return topological_object_order_;
	}

	const std::vector<SpatialConstraintId> &topologicalConstraintOrder() const
	{
		return topological_constraint_order_;
	}

	const std::vector<SpatialObjectId> &cyclePath() const { return cycle_path_; }
	const SpatialModelValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	std::vector<SpatialObjectId> topological_object_order_;
	std::vector<SpatialConstraintId> topological_constraint_order_;
	std::vector<SpatialObjectId> cycle_path_;
	SpatialModelValidationReport validation_report_;
};
