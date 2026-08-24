#pragma once

#include "vegetation/model/PlantInternode.h"

#include <string>
#include <utility>
#include <vector>

class PlantStem
{
public:
	PlantStem(
		std::string identifier,
		int branch_order,
		std::vector<PlantInternode> internodes)
		: identifier_(std::move(identifier)),
		  branch_order_(branch_order),
		  internodes_(std::move(internodes))
	{
	}

	const std::string &identifier() const { return identifier_; }
	int branchOrder() const { return branch_order_; }
	const std::vector<PlantInternode> &internodes() const { return internodes_; }
	const std::string &startNodeIdentifier() const
	{
		return internodes_.front().startNodeIdentifier();
	}
	const std::string &endNodeIdentifier() const
	{
		return internodes_.back().endNodeIdentifier();
	}
	float length() const
	{
		float total_length = 0.0f;
		for (const PlantInternode &internode : internodes_) {
			total_length += internode.length();
		}
		return total_length;
	}

private:
	std::string identifier_;
	int branch_order_ = 0;
	std::vector<PlantInternode> internodes_;
};
