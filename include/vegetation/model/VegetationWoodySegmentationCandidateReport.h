#pragma once

#include "vegetation/model/VegetationWoodyPointSegmentationValidationReport.h"
#include "vegetation/model/VegetationWoodySegmentationCandidate.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationWoodySegmentationCandidateIssueCode
{
	InvalidCandidateIdentifier,
	InvalidParameterSet,
	UnsupportedJobSchema,
	WrongReconstructionTarget,
	UnsupportedAlgorithm,
	DatasetIdentityMismatch,
	SourceHashMismatch,
	AdmissionRejected,
	InsufficientWoodyEvidence,
	CoverSetLimitExceeded,
	DisconnectedCoverGraph,
	AxisLimitExceeded,
	InvalidAxisCandidate,
	AmbiguousPointAssignment,
	SegmentationRejected
};

class VegetationWoodySegmentationCandidateIssue
{
public:
	VegetationWoodySegmentationCandidateIssue(
		VegetationWoodySegmentationCandidateIssueCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationWoodySegmentationCandidateIssueCode code() const { return code_; }
	const std::string &message() const { return message_; }

private:
	VegetationWoodySegmentationCandidateIssueCode code_ =
		VegetationWoodySegmentationCandidateIssueCode::InvalidParameterSet;
	std::string message_;
};

class VegetationWoodySegmentationCandidateReport
{
public:
	VegetationWoodySegmentationCandidateReport(
		std::vector<VegetationWoodySegmentationCandidateIssue> issues,
		std::optional<VegetationWoodyPointSegmentationValidationReport>
			segmentation_validation_report,
		std::optional<VegetationWoodySegmentationCandidate> candidate)
		: issues_(std::move(issues)),
		  segmentation_validation_report_(
			  std::move(segmentation_validation_report)),
		  candidate_(std::move(candidate))
	{
	}

	bool succeeded() const { return issues_.empty() && candidate_.has_value(); }
	const std::vector<VegetationWoodySegmentationCandidateIssue> &issues() const
	{
		return issues_;
	}
	const std::optional<VegetationWoodyPointSegmentationValidationReport> &
	segmentationValidationReport() const
	{
		return segmentation_validation_report_;
	}
	const std::optional<VegetationWoodySegmentationCandidate> &candidate() const
	{
		return candidate_;
	}

private:
	std::vector<VegetationWoodySegmentationCandidateIssue> issues_;
	std::optional<VegetationWoodyPointSegmentationValidationReport>
		segmentation_validation_report_;
	std::optional<VegetationWoodySegmentationCandidate> candidate_;
};
