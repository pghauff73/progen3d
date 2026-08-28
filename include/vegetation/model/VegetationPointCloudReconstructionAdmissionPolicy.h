#pragma once

#include "vegetation/model/VegetationPointCloudReconstructionTarget.h"

#include <cstddef>

class VegetationPointCloudReconstructionAdmissionPolicy
{
public:
	VegetationPointCloudReconstructionAdmissionPolicy(
		VegetationPointCloudReconstructionTarget target,
		std::size_t minimum_point_count,
		double minimum_axis_extent_metres,
		double maximum_duplicate_fraction,
		bool require_woody_and_foliage_labels)
		: target_(target),
		  minimum_point_count_(minimum_point_count),
		  minimum_axis_extent_metres_(minimum_axis_extent_metres),
		  maximum_duplicate_fraction_(maximum_duplicate_fraction),
		  require_woody_and_foliage_labels_(require_woody_and_foliage_labels)
	{
	}

	VegetationPointCloudReconstructionTarget target() const { return target_; }
	std::size_t minimumPointCount() const { return minimum_point_count_; }
	double minimumAxisExtentMetres() const
	{
		return minimum_axis_extent_metres_;
	}
	double maximumDuplicateFraction() const
	{
		return maximum_duplicate_fraction_;
	}
	bool requireWoodyAndFoliageLabels() const
	{
		return require_woody_and_foliage_labels_;
	}

private:
	VegetationPointCloudReconstructionTarget target_ =
		VegetationPointCloudReconstructionTarget::ShootArchitecture;
	std::size_t minimum_point_count_ = 0u;
	double minimum_axis_extent_metres_ = 0.0;
	double maximum_duplicate_fraction_ = 0.0;
	bool require_woody_and_foliage_labels_ = false;
};
