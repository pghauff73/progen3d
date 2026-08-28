#pragma once

#include "vegetation/model/VegetationWoodyBranchGraphReconstruction.h"
#include "vegetation/model/VegetationWoodySegmentationCandidate.h"
#include "vegetation/model/VegetationWoodySegmentationCandidateEvaluation.h"
#include "vegetation/model/VegetationWoodySegmentationSensitivityReport.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationAutomatedWoodySegmentationEnsembleIssueCode
{
	InvalidEnsembleIdentifier,
	UnsupportedJobSchema,
	WrongReconstructionTarget,
	UnsupportedAlgorithm,
	DatasetIdentityMismatch,
	SourceHashMismatch,
	AdmissionRejected,
	NoParameterCandidates,
	DuplicateParameterIdentifier,
	SensitivityGateRejected
};

class VegetationAutomatedWoodySegmentationEnsembleIssue
{
public:
	VegetationAutomatedWoodySegmentationEnsembleIssue(
		VegetationAutomatedWoodySegmentationEnsembleIssueCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationAutomatedWoodySegmentationEnsembleIssueCode code() const
	{
		return code_;
	}
	const std::string &message() const { return message_; }

private:
	VegetationAutomatedWoodySegmentationEnsembleIssueCode code_ =
		VegetationAutomatedWoodySegmentationEnsembleIssueCode::
			InvalidEnsembleIdentifier;
	std::string message_;
};

class VegetationAutomatedWoodySegmentationEnsembleReport
{
public:
	VegetationAutomatedWoodySegmentationEnsembleReport(
		std::vector<VegetationAutomatedWoodySegmentationEnsembleIssue> issues,
		std::vector<VegetationWoodySegmentationCandidateEvaluation> evaluations,
		std::optional<VegetationWoodySegmentationSensitivityReport>
			sensitivity_report,
		std::optional<VegetationWoodySegmentationCandidate> selected_candidate,
		std::optional<VegetationWoodyBranchGraphReconstruction>
			selected_reconstruction)
		: issues_(std::move(issues)),
		  evaluations_(std::move(evaluations)),
		  sensitivity_report_(std::move(sensitivity_report)),
		  selected_candidate_(std::move(selected_candidate)),
		  selected_reconstruction_(std::move(selected_reconstruction))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && sensitivity_report_.has_value() &&
		       sensitivity_report_->acceptedForGraphSelection() &&
		       selected_candidate_.has_value() &&
		       selected_reconstruction_.has_value();
	}
	const std::vector<VegetationAutomatedWoodySegmentationEnsembleIssue> &issues()
		const
	{
		return issues_;
	}
	const std::vector<VegetationWoodySegmentationCandidateEvaluation> &
	evaluations() const
	{
		return evaluations_;
	}
	const std::optional<VegetationWoodySegmentationSensitivityReport> &
	sensitivityReport() const
	{
		return sensitivity_report_;
	}
	const std::optional<VegetationWoodySegmentationCandidate> &selectedCandidate()
		const
	{
		return selected_candidate_;
	}
	const std::optional<VegetationWoodyBranchGraphReconstruction> &
	selectedReconstruction() const
	{
		return selected_reconstruction_;
	}

private:
	std::vector<VegetationAutomatedWoodySegmentationEnsembleIssue> issues_;
	std::vector<VegetationWoodySegmentationCandidateEvaluation> evaluations_;
	std::optional<VegetationWoodySegmentationSensitivityReport>
		sensitivity_report_;
	std::optional<VegetationWoodySegmentationCandidate> selected_candidate_;
	std::optional<VegetationWoodyBranchGraphReconstruction>
		selected_reconstruction_;
};
