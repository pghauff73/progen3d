#pragma once

#include "vegetation/model/BranchNodeState.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

class BranchNode
{
public:
	BranchNode(std::string identifier,
	           glm::vec3 position,
	           float radius,
	           float developmental_age,
	           int branch_order,
	           BranchNodeState state)
		: identifier_(std::move(identifier)),
		  position_(position),
		  radius_(radius),
		  developmental_age_(developmental_age),
		  branch_order_(branch_order),
		  state_(state)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &position() const { return position_; }
	float radius() const { return radius_; }
	float developmentalAge() const { return developmental_age_; }
	int branchOrder() const { return branch_order_; }
	BranchNodeState state() const { return state_; }

private:
	std::string identifier_;
	glm::vec3 position_{0.0f};
	float radius_ = 0.0f;
	float developmental_age_ = 0.0f;
	int branch_order_ = 0;
	BranchNodeState state_ = BranchNodeState::Dormant;
};

