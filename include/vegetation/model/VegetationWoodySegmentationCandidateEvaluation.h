#pragma once

#include "vegetation/model/VegetationWoodyBranchGraphReconstructionReport.h"
#include "vegetation/model/VegetationWoodySegmentationCandidateReport.h"
#include "vegetation/model/VegetationWoodySegmentationParameterSet.h"

#include <optional>
#include <utility>

class VegetationWoodySegmentationCandidateEvaluation
{
public:
	VegetationWoodySegmentationCandidateEvaluation(
		VegetationWoodySegmentationParameterSet parameter_set,
		VegetationWoodySegmentationCandidateReport segmentation_report,
		std::optional<VegetationWoodyBranchGraphReconstructionReport>
			graph_report,
		double total_axis_length_metres,
		double woody_volume_cubic_metres)
		: parameter_set_(std::move(parameter_set)),
		  segmentation_report_(std::move(segmentation_report)),
		  graph_report_(std::move(graph_report)),
		  total_axis_length_metres_(total_axis_length_metres),
		  woody_volume_cubic_metres_(woody_volume_cubic_metres)
	{
	}

	bool acceptedForSensitivity() const
	{
		return segmentation_report_.succeeded() && graph_report_.has_value() &&
		       graph_report_->succeeded();
	}
	const VegetationWoodySegmentationParameterSet &parameterSet() const
	{
		return parameter_set_;
	}
	const VegetationWoodySegmentationCandidateReport &segmentationReport() const
	{
		return segmentation_report_;
	}
	const std::optional<VegetationWoodyBranchGraphReconstructionReport> &
	graphReport() const
	{
		return graph_report_;
	}
	double totalAxisLengthMetres() const
	{
		return total_axis_length_metres_;
	}
	double woodyVolumeCubicMetres() const
	{
		return woody_volume_cubic_metres_;
	}

private:
	VegetationWoodySegmentationParameterSet parameter_set_;
	VegetationWoodySegmentationCandidateReport segmentation_report_;
	std::optional<VegetationWoodyBranchGraphReconstructionReport> graph_report_;
	double total_axis_length_metres_ = 0.0;
	double woody_volume_cubic_metres_ = 0.0;
};
