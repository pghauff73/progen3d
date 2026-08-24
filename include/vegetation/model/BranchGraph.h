#pragma once

#include "vegetation/model/BranchNode.h"
#include "vegetation/model/BranchSegment.h"
#include "vegetation/model/OrganAttachment.h"
#include "vegetation/model/VegetationBud.h"
#include "vegetation/model/VegetationGrowthTip.h"

#include <string>
#include <utility>
#include <vector>

class BranchGraph
{
public:
	BranchGraph(std::string root_node_identifier,
	            std::vector<BranchNode> nodes,
	            std::vector<BranchSegment> segments,
	            std::vector<VegetationBud> buds = {},
	            std::vector<OrganAttachment> organ_attachments = {},
	            std::vector<VegetationGrowthTip> growth_tips = {})
		: root_node_identifier_(std::move(root_node_identifier)),
		  nodes_(std::move(nodes)),
		  segments_(std::move(segments)),
		  buds_(std::move(buds)),
		  organ_attachments_(std::move(organ_attachments)),
		  growth_tips_(std::move(growth_tips))
	{
	}

	const std::string &rootNodeIdentifier() const
	{
		return root_node_identifier_;
	}
	const std::vector<BranchNode> &nodes() const { return nodes_; }
	const std::vector<BranchSegment> &segments() const { return segments_; }
	const std::vector<VegetationBud> &buds() const { return buds_; }
	const std::vector<OrganAttachment> &organAttachments() const
	{
		return organ_attachments_;
	}
	const std::vector<VegetationGrowthTip> &growthTips() const
	{
		return growth_tips_;
	}

private:
	std::string root_node_identifier_;
	std::vector<BranchNode> nodes_;
	std::vector<BranchSegment> segments_;
	std::vector<VegetationBud> buds_;
	std::vector<OrganAttachment> organ_attachments_;
	std::vector<VegetationGrowthTip> growth_tips_;
};

