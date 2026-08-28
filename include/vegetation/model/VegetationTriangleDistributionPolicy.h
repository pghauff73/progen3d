#pragma once

#include "vegetation/model/VegetationTriangleImportance.h"

#include <cstddef>
#include <utility>

class VegetationTriangleDistributionPolicy
{
public:
	VegetationTriangleDistributionPolicy(
		std::size_t target_triangle_count,
		std::size_t maximum_meshlet_triangle_count = 124u,
		bool preserve_organ_area = true,
		VegetationTriangleImportanceWeights importance_weights =
			VegetationTriangleImportanceWeights())
		: target_triangle_count_(target_triangle_count),
		  maximum_meshlet_triangle_count_(maximum_meshlet_triangle_count),
		  preserve_organ_area_(preserve_organ_area),
		  importance_weights_(std::move(importance_weights))
	{
	}

	std::size_t targetTriangleCount() const { return target_triangle_count_; }
	std::size_t maximumMeshletTriangleCount() const
	{
		return maximum_meshlet_triangle_count_;
	}
	bool preservesOrganArea() const { return preserve_organ_area_; }
	const VegetationTriangleImportanceWeights &importanceWeights() const
	{
		return importance_weights_;
	}

private:
	std::size_t target_triangle_count_ = 0u;
	std::size_t maximum_meshlet_triangle_count_ = 124u;
	bool preserve_organ_area_ = true;
	VegetationTriangleImportanceWeights importance_weights_;
};
