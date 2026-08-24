#pragma once

#include "vegetation/model/BranchSegmentKind.h"

#include <string>
#include <utility>

class BranchSegment
{
public:
	BranchSegment(std::string identifier,
	              std::string parent_node_identifier,
	              std::string child_node_identifier,
	              BranchSegmentKind kind)
		: identifier_(std::move(identifier)),
		  parent_node_identifier_(std::move(parent_node_identifier)),
		  child_node_identifier_(std::move(child_node_identifier)),
		  kind_(kind)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &parentNodeIdentifier() const
	{
		return parent_node_identifier_;
	}
	const std::string &childNodeIdentifier() const
	{
		return child_node_identifier_;
	}
	BranchSegmentKind kind() const { return kind_; }

private:
	std::string identifier_;
	std::string parent_node_identifier_;
	std::string child_node_identifier_;
	BranchSegmentKind kind_ = BranchSegmentKind::Continuation;
};

