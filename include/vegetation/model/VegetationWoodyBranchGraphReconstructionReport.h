#pragma once

#include "vegetation/model/VegetationWoodyBranchGraphQualityReport.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstruction.h"
#include "vegetation/model/VegetationWoodyPointSegmentationValidationReport.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationWoodyBranchGraphReconstructionIssueCode
{
	InvalidPolicy,
	InvalidReconstructionIdentifier,
	UnsupportedJobSchema,
	WrongReconstructionTarget,
	UnsupportedAlgorithm,
	DatasetIdentityMismatch,
	SourceHashMismatch,
	AdmissionRejected,
	SegmentationRejected,
	SegmentationIncomplete,
	AxisLimitExceeded,
	InsufficientAxisEvidence,
	AxisFitFailed,
	InsufficientAxialStations,
	InsufficientRadialEvidence,
	CylinderLimitExceeded,
	ParentAxisUnavailable,
	QualityGateRejected
};

class VegetationWoodyBranchGraphReconstructionIssue
{
public:
	VegetationWoodyBranchGraphReconstructionIssue(
		VegetationWoodyBranchGraphReconstructionIssueCode code,
		std::string axis_identifier,
		std::string message)
		: code_(code),
		  axis_identifier_(std::move(axis_identifier)),
		  message_(std::move(message))
	{
	}

	VegetationWoodyBranchGraphReconstructionIssueCode code() const
	{
		return code_;
	}
	const std::string &axisIdentifier() const { return axis_identifier_; }
	const std::string &message() const { return message_; }

private:
	VegetationWoodyBranchGraphReconstructionIssueCode code_ =
		VegetationWoodyBranchGraphReconstructionIssueCode::InvalidPolicy;
	std::string axis_identifier_;
	std::string message_;
};

class VegetationWoodyBranchGraphReconstructionReport
{
public:
	VegetationWoodyBranchGraphReconstructionReport(
		std::vector<VegetationWoodyBranchGraphReconstructionIssue> issues,
		std::optional<VegetationWoodyPointSegmentationValidationReport>
			segmentation_report,
		std::optional<VegetationWoodyBranchGraphQualityReport> quality_report,
		std::optional<VegetationWoodyBranchGraphReconstruction> reconstruction)
		: issues_(std::move(issues)),
		  segmentation_report_(std::move(segmentation_report)),
		  quality_report_(std::move(quality_report)),
		  reconstruction_(std::move(reconstruction))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && reconstruction_.has_value();
	}
	const std::vector<VegetationWoodyBranchGraphReconstructionIssue> &issues()
		const
	{
		return issues_;
	}
	const std::optional<VegetationWoodyPointSegmentationValidationReport> &
	segmentationReport() const
	{
		return segmentation_report_;
	}
	const std::optional<VegetationWoodyBranchGraphQualityReport> &qualityReport()
		const
	{
		return quality_report_;
	}
	const std::optional<VegetationWoodyBranchGraphReconstruction> &
	reconstruction() const
	{
		return reconstruction_;
	}

private:
	std::vector<VegetationWoodyBranchGraphReconstructionIssue> issues_;
	std::optional<VegetationWoodyPointSegmentationValidationReport>
		segmentation_report_;
	std::optional<VegetationWoodyBranchGraphQualityReport> quality_report_;
	std::optional<VegetationWoodyBranchGraphReconstruction> reconstruction_;
};
