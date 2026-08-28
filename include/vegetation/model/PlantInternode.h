#pragma once

#include "vegetation/model/BranchSegmentKind.h"

#include <glm/glm.hpp>
#include <glm/geometric.hpp>

#include <string>
#include <utility>

class PlantInternode
{
public:
	PlantInternode(
		std::string identifier,
		std::string source_segment_identifier,
		std::string start_node_identifier,
		std::string end_node_identifier,
		glm::vec3 start_position,
		glm::vec3 end_position,
		float start_radius,
		float end_radius,
		int branch_order,
		BranchSegmentKind segment_kind)
		: identifier_(std::move(identifier)),
		  source_segment_identifier_(std::move(source_segment_identifier)),
		  start_node_identifier_(std::move(start_node_identifier)),
		  end_node_identifier_(std::move(end_node_identifier)),
		  start_position_(start_position),
		  end_position_(end_position),
		  start_radius_(start_radius),
		  end_radius_(end_radius),
		  branch_order_(branch_order),
		  segment_kind_(segment_kind)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &sourceSegmentIdentifier() const
	{
		return source_segment_identifier_;
	}
	const std::string &startNodeIdentifier() const
	{
		return start_node_identifier_;
	}
	const std::string &endNodeIdentifier() const { return end_node_identifier_; }
	const glm::vec3 &startPosition() const { return start_position_; }
	const glm::vec3 &endPosition() const { return end_position_; }
	float startRadius() const { return start_radius_; }
	float endRadius() const { return end_radius_; }
	int branchOrder() const { return branch_order_; }
	BranchSegmentKind segmentKind() const { return segment_kind_; }
	float length() const { return glm::length(end_position_ - start_position_); }

private:
	std::string identifier_;
	std::string source_segment_identifier_;
	std::string start_node_identifier_;
	std::string end_node_identifier_;
	glm::vec3 start_position_{0.0f};
	glm::vec3 end_position_{0.0f};
	float start_radius_ = 0.0f;
	float end_radius_ = 0.0f;
	int branch_order_ = 0;
	BranchSegmentKind segment_kind_ = BranchSegmentKind::Continuation;
};
