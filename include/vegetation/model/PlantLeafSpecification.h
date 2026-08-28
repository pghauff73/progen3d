#pragma once

#include "geometry/model/BotanicalBladeProfile.h"
#include "vegetation/model/PhyllotaxisSpecification.h"
#include "vegetation/model/PlantCompoundLeafSpecification.h"
#include "vegetation/model/PlantPetioleSpecification.h"

class PlantLeafSpecification
{
public:
	PlantLeafSpecification(
		BotanicalBladeProfile profile,
		float length,
		float width,
		float camber,
		float twist_degrees,
		float thickness,
		int minimum_branch_order,
		PlantPetioleSpecification petiole,
		PhyllotaxisSpecification phyllotaxis)
		: PlantLeafSpecification(
			profile, length, width, camber, twist_degrees, thickness,
			minimum_branch_order, petiole, PlantCompoundLeafSpecification(),
			phyllotaxis)
	{
	}

	PlantLeafSpecification(
		BotanicalBladeProfile profile,
		float length,
		float width,
		float camber,
		float twist_degrees,
		float thickness,
		int minimum_branch_order,
		PlantPetioleSpecification petiole,
		PlantCompoundLeafSpecification compound_leaf,
		PhyllotaxisSpecification phyllotaxis)
		: profile_(profile),
		  length_(length),
		  width_(width),
		  camber_(camber),
		  twist_degrees_(twist_degrees),
		  thickness_(thickness),
		  minimum_branch_order_(minimum_branch_order),
		  petiole_(petiole),
		  compound_leaf_(compound_leaf),
		  phyllotaxis_(phyllotaxis)
	{
	}

	BotanicalBladeProfile profile() const { return profile_; }
	float length() const { return length_; }
	float width() const { return width_; }
	float camber() const { return camber_; }
	float twistDegrees() const { return twist_degrees_; }
	float thickness() const { return thickness_; }
	int minimumBranchOrder() const { return minimum_branch_order_; }
	const PlantPetioleSpecification &petiole() const { return petiole_; }
	const PlantCompoundLeafSpecification &compoundLeaf() const
	{
		return compound_leaf_;
	}
	const PhyllotaxisSpecification &phyllotaxis() const
	{
		return phyllotaxis_;
	}

private:
	BotanicalBladeProfile profile_ = BotanicalBladeProfile::Elliptic;
	float length_ = 0.10f;
	float width_ = 0.05f;
	float camber_ = 0.0f;
	float twist_degrees_ = 0.0f;
	float thickness_ = 0.001f;
	int minimum_branch_order_ = 1;
	PlantPetioleSpecification petiole_{0.02f, 0.002f, 0.001f};
	PlantCompoundLeafSpecification compound_leaf_;
	PhyllotaxisSpecification phyllotaxis_{
		PhyllotaxisMode::Alternate, 137.5f, 0.04f};
};
