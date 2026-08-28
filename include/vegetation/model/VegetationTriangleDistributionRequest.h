#pragma once

#include "vegetation/model/VegetationTriangleDistributionPolicy.h"
#include "vegetation/model/VegetationTriangleRegion.h"

#include <utility>
#include <vector>

class VegetationTriangleDistributionRequest
{
public:
	VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy policy,
		std::vector<VegetationTriangleRegion> regions)
		: policy_(std::move(policy)), regions_(std::move(regions))
	{
	}

	const VegetationTriangleDistributionPolicy &policy() const { return policy_; }
	const std::vector<VegetationTriangleRegion> &regions() const { return regions_; }

private:
	VegetationTriangleDistributionPolicy policy_;
	std::vector<VegetationTriangleRegion> regions_;
};
