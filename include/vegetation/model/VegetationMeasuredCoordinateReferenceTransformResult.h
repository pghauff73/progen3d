#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <optional>
#include <string>
#include <utility>

class VegetationMeasuredCoordinateReferenceTransformResult
{
public:
	VegetationMeasuredCoordinateReferenceTransformResult(
		std::optional<VegetationMeasuredPoint3d> transformed_point,
		std::string target_coordinate_system,
		std::string target_coordinate_unit,
		std::string rejection_reason)
		: transformed_point_(std::move(transformed_point)),
		  target_coordinate_system_(std::move(target_coordinate_system)),
		  target_coordinate_unit_(std::move(target_coordinate_unit)),
		  rejection_reason_(std::move(rejection_reason))
	{
	}

	bool succeeded() const { return transformed_point_.has_value(); }
	const std::optional<VegetationMeasuredPoint3d> &transformedPoint() const
	{
		return transformed_point_;
	}
	const std::string &targetCoordinateSystem() const
	{
		return target_coordinate_system_;
	}
	const std::string &targetCoordinateUnit() const
	{
		return target_coordinate_unit_;
	}
	const std::string &rejectionReason() const { return rejection_reason_; }

private:
	std::optional<VegetationMeasuredPoint3d> transformed_point_;
	std::string target_coordinate_system_;
	std::string target_coordinate_unit_;
	std::string rejection_reason_;
};
