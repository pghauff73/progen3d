#pragma once

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/PlantBranchingSpecification.h"
#include "vegetation/model/PlantFlowerSpecification.h"
#include "vegetation/model/PlantFruitSpecification.h"
#include "vegetation/model/PlantLeafSpecification.h"
#include "vegetation/model/PlantOrganArraySpecification.h"

#include <string>
#include <utility>
#include <vector>

class PlantSpeciesSpecification
{
public:
	PlantSpeciesSpecification(
		std::string identifier,
		PlantArchitecture architecture,
		PlantBranchingSpecification branching,
		PlantLeafSpecification leaf,
		float flowering_age,
		float mature_age,
		PlantFlowerSpecification flower = PlantFlowerSpecification(),
		std::vector<PlantOrganArraySpecification> organ_arrays = {},
		PlantFruitSpecification fruit = PlantFruitSpecification())
		: identifier_(std::move(identifier)),
		  architecture_(architecture),
		  branching_(branching),
		  leaf_(leaf),
		  flowering_age_(flowering_age),
		  mature_age_(mature_age),
		  flower_(flower),
		  organ_arrays_(std::move(organ_arrays)),
		  fruit_(fruit)
	{
	}

	const std::string &identifier() const { return identifier_; }
	PlantArchitecture architecture() const { return architecture_; }
	const PlantBranchingSpecification &branching() const { return branching_; }
	const PlantLeafSpecification &leaf() const { return leaf_; }
	float floweringAge() const { return flowering_age_; }
	float matureAge() const { return mature_age_; }
	const PlantFlowerSpecification &flower() const { return flower_; }
	const std::vector<PlantOrganArraySpecification> &organArrays() const
	{
		return organ_arrays_;
	}
	const PlantFruitSpecification &fruit() const { return fruit_; }

private:
	std::string identifier_;
	PlantArchitecture architecture_ = PlantArchitecture::Tree;
	PlantBranchingSpecification branching_{
		1.0f, 0.1f, 0.01f, 6, 2, 1, 2, 42.0f, 137.5f,
		0.42f, 0.62f, 0.88f, 2.0f, 4.0f, 0.08f, 0.15f};
	PlantLeafSpecification leaf_{
		BotanicalBladeProfile::Elliptic, 0.1f, 0.05f, 0.0f, 0.0f,
		0.001f, 1,
		PlantPetioleSpecification(0.02f, 0.002f, 0.001f),
		PlantCompoundLeafSpecification(),
		PhyllotaxisSpecification(
			PhyllotaxisMode::Alternate, 137.5f, 0.04f)};
	float flowering_age_ = 1.0f;
	float mature_age_ = 1.0f;
	PlantFlowerSpecification flower_;
	std::vector<PlantOrganArraySpecification> organ_arrays_;
	PlantFruitSpecification fruit_;
};
