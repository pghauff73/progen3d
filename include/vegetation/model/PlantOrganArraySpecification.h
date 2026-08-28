#pragma once

#include "vegetation/model/VegetationOrganArrayHost.h"
#include "vegetation/model/VegetationOrganOrientation.h"
#include "vegetation/model/VegetationOrganType.h"

#include <string>
#include <utility>

class PlantOrganArraySpecification
{
public:
	PlantOrganArraySpecification(
		std::string identifier,
		VegetationOrganType organ_type,
		VegetationOrganArrayHost host,
		int count,
		float spacing,
		float azimuth_progression_degrees,
		float initial_scale,
		float scale_falloff,
		VegetationOrganOrientation orientation,
		float jitter_fraction,
		int minimum_branch_order = 0)
		: identifier_(std::move(identifier)),
		  organ_type_(organ_type),
		  host_(host),
		  count_(count),
		  spacing_(spacing),
		  azimuth_progression_degrees_(azimuth_progression_degrees),
		  initial_scale_(initial_scale),
		  scale_falloff_(scale_falloff),
		  orientation_(orientation),
		  jitter_fraction_(jitter_fraction),
		  minimum_branch_order_(minimum_branch_order)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VegetationOrganType organType() const { return organ_type_; }
	VegetationOrganArrayHost host() const { return host_; }
	int count() const { return count_; }
	float spacing() const { return spacing_; }
	float azimuthProgressionDegrees() const
	{
		return azimuth_progression_degrees_;
	}
	float initialScale() const { return initial_scale_; }
	float scaleFalloff() const { return scale_falloff_; }
	VegetationOrganOrientation orientation() const { return orientation_; }
	float jitterFraction() const { return jitter_fraction_; }
	int minimumBranchOrder() const { return minimum_branch_order_; }

private:
	std::string identifier_;
	VegetationOrganType organ_type_ = VegetationOrganType::Leaf;
	VegetationOrganArrayHost host_ = VegetationOrganArrayHost::Branch;
	int count_ = 1;
	float spacing_ = 0.0f;
	float azimuth_progression_degrees_ = 137.50776f;
	float initial_scale_ = 1.0f;
	float scale_falloff_ = 1.0f;
	VegetationOrganOrientation orientation_ =
		VegetationOrganOrientation::Outward;
	float jitter_fraction_ = 0.0f;
	int minimum_branch_order_ = 0;
};
