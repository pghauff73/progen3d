#pragma once

#include "spatial/model/SpatialModelValidationReport.h"
#include "spatial/model/SpatialResolutionRecord.h"

#include <optional>
#include <utility>

#include <glm/glm.hpp>

class SpatialConstraintResolutionResult {
public:
	static SpatialConstraintResolutionResult succeeded(
		glm::mat4 resolution_local_transform,
		glm::mat4 final_world_transform,
		SpatialResolutionRecord resolution_record)
	{
		return SpatialConstraintResolutionResult(
			resolution_local_transform,
			final_world_transform,
			std::move(resolution_record),
			SpatialModelValidationReport());
	}

	static SpatialConstraintResolutionResult failed(
		SpatialModelValidationReport validation_report)
	{
		return SpatialConstraintResolutionResult(
			std::nullopt,
			std::nullopt,
			std::nullopt,
			std::move(validation_report));
	}

	bool succeeded() const
	{
		return resolution_local_transform_.has_value() &&
		       final_world_transform_.has_value() &&
		       resolution_record_.has_value() &&
		       validation_report_.isValid();
	}

	const std::optional<glm::mat4> &resolutionLocalTransform() const
	{
		return resolution_local_transform_;
	}
	const std::optional<glm::mat4> &finalWorldTransform() const
	{
		return final_world_transform_;
	}
	const std::optional<SpatialResolutionRecord> &resolutionRecord() const
	{
		return resolution_record_;
	}
	const SpatialModelValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	SpatialConstraintResolutionResult(
		std::optional<glm::mat4> resolution_local_transform,
		std::optional<glm::mat4> final_world_transform,
		std::optional<SpatialResolutionRecord> resolution_record,
		SpatialModelValidationReport validation_report)
		: resolution_local_transform_(std::move(resolution_local_transform)),
		  final_world_transform_(std::move(final_world_transform)),
		  resolution_record_(std::move(resolution_record)),
		  validation_report_(std::move(validation_report)) {}

	std::optional<glm::mat4> resolution_local_transform_;
	std::optional<glm::mat4> final_world_transform_;
	std::optional<SpatialResolutionRecord> resolution_record_;
	SpatialModelValidationReport validation_report_;
};
