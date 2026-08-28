#pragma once

#include "vegetation/model/VegetationWoodyAxisQualityReport.h"
#include "vegetation/model/VegetationWoodyAxisReconstruction.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationWoodyAxisReconstructionIssueCode
{
	InvalidPolicy,
	InvalidReconstructionIdentifier,
	UnsupportedJobSchema,
	WrongReconstructionTarget,
	UnsupportedAlgorithm,
	DatasetIdentityMismatch,
	SourceHashMismatch,
	AdmissionRejected,
	InsufficientWoodyEvidence,
	InsufficientTrainingEvidence,
	PrincipalAxisSolutionFailed,
	InvalidPointExtent,
	InsufficientRadialEvidence,
	InsufficientAxialStations,
	CylinderLimitExceeded,
	QualityGateRejected
};

class VegetationWoodyAxisReconstructionIssue
{
public:
	VegetationWoodyAxisReconstructionIssue(
		VegetationWoodyAxisReconstructionIssueCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationWoodyAxisReconstructionIssueCode code() const { return code_; }
	const std::string &message() const { return message_; }

private:
	VegetationWoodyAxisReconstructionIssueCode code_ =
		VegetationWoodyAxisReconstructionIssueCode::InvalidPolicy;
	std::string message_;
};

class VegetationWoodyAxisReconstructionReport
{
public:
	VegetationWoodyAxisReconstructionReport(
		std::vector<VegetationWoodyAxisReconstructionIssue> issues,
		std::optional<VegetationWoodyAxisQualityReport> quality_report,
		std::optional<VegetationWoodyAxisReconstruction> reconstruction)
		: issues_(std::move(issues)),
		  quality_report_(std::move(quality_report)),
		  reconstruction_(std::move(reconstruction))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && reconstruction_.has_value();
	}
	const std::vector<VegetationWoodyAxisReconstructionIssue> &issues() const
	{
		return issues_;
	}
	const std::optional<VegetationWoodyAxisQualityReport> &qualityReport() const
	{
		return quality_report_;
	}
	const std::optional<VegetationWoodyAxisReconstruction> &reconstruction()
		const
	{
		return reconstruction_;
	}

private:
	std::vector<VegetationWoodyAxisReconstructionIssue> issues_;
	std::optional<VegetationWoodyAxisQualityReport> quality_report_;
	std::optional<VegetationWoodyAxisReconstruction> reconstruction_;
};
