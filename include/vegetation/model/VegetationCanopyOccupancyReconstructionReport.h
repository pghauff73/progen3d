#pragma once

#include "vegetation/model/VegetationCanopyOccupancyQualityReport.h"
#include "vegetation/model/VegetationCanopyOccupancyReconstruction.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationCanopyOccupancyReconstructionIssueCode
{
	InvalidPolicy,
	InvalidReconstructionIdentifier,
	UnsupportedJobSchema,
	WrongReconstructionTarget,
	UnsupportedAlgorithm,
	DatasetIdentityMismatch,
	SourceHashMismatch,
	AdmissionRejected,
	InsufficientFoliageEvidence,
	InvalidPointExtent,
	OccupiedCellLimitExceeded,
	QualityGateRejected
};

class VegetationCanopyOccupancyReconstructionIssue
{
public:
	VegetationCanopyOccupancyReconstructionIssue(
		VegetationCanopyOccupancyReconstructionIssueCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationCanopyOccupancyReconstructionIssueCode code() const
	{
		return code_;
	}
	const std::string &message() const { return message_; }

private:
	VegetationCanopyOccupancyReconstructionIssueCode code_ =
		VegetationCanopyOccupancyReconstructionIssueCode::InvalidPolicy;
	std::string message_;
};

class VegetationCanopyOccupancyReconstructionReport
{
public:
	VegetationCanopyOccupancyReconstructionReport(
		std::vector<VegetationCanopyOccupancyReconstructionIssue> issues,
		std::optional<VegetationCanopyOccupancyQualityReport> quality_report,
		std::optional<VegetationCanopyOccupancyReconstruction> reconstruction)
		: issues_(std::move(issues)),
		  quality_report_(std::move(quality_report)),
		  reconstruction_(std::move(reconstruction))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && reconstruction_.has_value();
	}
	const std::vector<VegetationCanopyOccupancyReconstructionIssue> &issues() const
	{
		return issues_;
	}
	const std::optional<VegetationCanopyOccupancyQualityReport> &qualityReport()
		const
	{
		return quality_report_;
	}
	const std::optional<VegetationCanopyOccupancyReconstruction> &reconstruction()
		const
	{
		return reconstruction_;
	}

private:
	std::vector<VegetationCanopyOccupancyReconstructionIssue> issues_;
	std::optional<VegetationCanopyOccupancyQualityReport> quality_report_;
	std::optional<VegetationCanopyOccupancyReconstruction> reconstruction_;
};
