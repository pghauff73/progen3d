#pragma once

#include "vegetation/model/VegetationWoodyCoverConnection.h"
#include "vegetation/model/VegetationWoodyCoverSet.h"
#include "vegetation/model/VegetationWoodyPointSegmentation.h"
#include "vegetation/model/VegetationWoodySegmentationAxisCandidate.h"
#include "vegetation/model/VegetationWoodySegmentationParameterSet.h"

#include <string>
#include <utility>
#include <vector>

class VegetationWoodySegmentationCandidate
{
public:
	VegetationWoodySegmentationCandidate(
		std::string candidate_identifier,
		VegetationWoodySegmentationParameterSet parameter_set,
		std::string root_cover_identifier,
		std::vector<VegetationWoodyCoverSet> cover_sets,
		std::vector<VegetationWoodyCoverConnection> cover_connections,
		std::vector<VegetationWoodySegmentationAxisCandidate> axis_candidates,
		VegetationWoodyPointSegmentation segmentation)
		: candidate_identifier_(std::move(candidate_identifier)),
		  parameter_set_(std::move(parameter_set)),
		  root_cover_identifier_(std::move(root_cover_identifier)),
		  cover_sets_(std::move(cover_sets)),
		  cover_connections_(std::move(cover_connections)),
		  axis_candidates_(std::move(axis_candidates)),
		  segmentation_(std::move(segmentation))
	{
	}

	const std::string &candidateIdentifier() const
	{
		return candidate_identifier_;
	}
	const VegetationWoodySegmentationParameterSet &parameterSet() const
	{
		return parameter_set_;
	}
	const std::string &rootCoverIdentifier() const
	{
		return root_cover_identifier_;
	}
	const std::vector<VegetationWoodyCoverSet> &coverSets() const
	{
		return cover_sets_;
	}
	const std::vector<VegetationWoodyCoverConnection> &coverConnections() const
	{
		return cover_connections_;
	}
	const std::vector<VegetationWoodySegmentationAxisCandidate> &axisCandidates()
		const
	{
		return axis_candidates_;
	}
	const VegetationWoodyPointSegmentation &segmentation() const
	{
		return segmentation_;
	}

private:
	std::string candidate_identifier_;
	VegetationWoodySegmentationParameterSet parameter_set_;
	std::string root_cover_identifier_;
	std::vector<VegetationWoodyCoverSet> cover_sets_;
	std::vector<VegetationWoodyCoverConnection> cover_connections_;
	std::vector<VegetationWoodySegmentationAxisCandidate> axis_candidates_;
	VegetationWoodyPointSegmentation segmentation_;
};
