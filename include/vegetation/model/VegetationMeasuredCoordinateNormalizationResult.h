#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <optional>
#include <string>
#include <utility>

class VegetationMeasuredCoordinateNormalizationResult
{
public:
	VegetationMeasuredCoordinateNormalizationResult(
		std::optional<VegetationMeasuredPoint3d> normalized_point,
		std::string canonical_coordinate_system,
		std::string rejection_reason)
		: normalized_point_(normalized_point),
		  canonical_coordinate_system_(
			  std::move(canonical_coordinate_system)),
		  rejection_reason_(std::move(rejection_reason))
	{
	}

	bool succeeded() const { return normalized_point_.has_value(); }
	const std::optional<VegetationMeasuredPoint3d> &normalizedPoint() const
	{
		return normalized_point_;
	}
	const std::string &canonicalCoordinateSystem() const
	{
		return canonical_coordinate_system_;
	}
	const std::string &rejectionReason() const { return rejection_reason_; }

private:
	std::optional<VegetationMeasuredPoint3d> normalized_point_;
	std::string canonical_coordinate_system_;
	std::string rejection_reason_;
};
