#pragma once

#include "vegetation/model/VegetationOrganType.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

enum class VegetationBudState
{
	Dormant,
	Active,
	FlowerBud,
	LeafBud,
	BranchBud,
	Dead
};

class VegetationBud
{
public:
	VegetationBud(std::string identifier,
	              std::string host_node_identifier,
	              glm::vec3 direction,
	              float developmental_age,
	              VegetationBudState state,
	              VegetationOrganType organ_type,
	              float activation_probability)
		: identifier_(std::move(identifier)),
		  host_node_identifier_(std::move(host_node_identifier)),
		  direction_(direction),
		  developmental_age_(developmental_age),
		  state_(state),
		  organ_type_(organ_type),
		  activation_probability_(activation_probability)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostNodeIdentifier() const
	{
		return host_node_identifier_;
	}
	const glm::vec3 &direction() const { return direction_; }
	float developmentalAge() const { return developmental_age_; }
	VegetationBudState state() const { return state_; }
	VegetationOrganType organType() const { return organ_type_; }
	float activationProbability() const { return activation_probability_; }

private:
	std::string identifier_;
	std::string host_node_identifier_;
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	float developmental_age_ = 0.0f;
	VegetationBudState state_ = VegetationBudState::Dormant;
	VegetationOrganType organ_type_ = VegetationOrganType::Leaf;
	float activation_probability_ = 0.0f;
};

