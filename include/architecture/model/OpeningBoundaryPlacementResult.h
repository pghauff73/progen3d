#pragma once

#include "spatial/model/SpatialResolutionRecord.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <utility>

class OpeningBoundaryPlacementResult
{
public:
	static OpeningBoundaryPlacementResult succeeded(
		glm::mat4 local_transform,
		glm::vec4 perimeter_gaps,
		SpatialResolutionRecord resolution_record)
	{
		return OpeningBoundaryPlacementResult(
			local_transform,
			perimeter_gaps,
			std::move(resolution_record),
			{});
	}

	static OpeningBoundaryPlacementResult failed(std::string diagnostic)
	{
		return OpeningBoundaryPlacementResult(
			std::nullopt, glm::vec4(0.0f), std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const
	{
		return local_transform_.has_value() && resolution_record_.has_value();
	}
	const std::optional<glm::mat4> &localTransform() const { return local_transform_; }
	const glm::vec4 &perimeterGaps() const { return perimeter_gaps_; }
	const std::optional<SpatialResolutionRecord> &resolutionRecord() const
	{
		return resolution_record_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	OpeningBoundaryPlacementResult(
		std::optional<glm::mat4> local_transform,
		glm::vec4 perimeter_gaps,
		std::optional<SpatialResolutionRecord> resolution_record,
		std::string diagnostic)
		: local_transform_(std::move(local_transform)),
		  perimeter_gaps_(perimeter_gaps),
		  resolution_record_(std::move(resolution_record)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<glm::mat4> local_transform_;
	glm::vec4 perimeter_gaps_{0.0f};
	std::optional<SpatialResolutionRecord> resolution_record_;
	std::string diagnostic_;
};
