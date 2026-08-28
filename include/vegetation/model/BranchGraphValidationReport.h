#pragma once

#include <string>
#include <utility>
#include <vector>

enum class BranchGraphDiagnosticCode
{
	ComplexityLimitExceeded,
	EmptyIdentifier,
	DuplicateIdentifier,
	NonFiniteValue,
	InvalidRadius,
	InvalidDevelopmentalAge,
	InvalidBranchOrder,
	MissingRoot,
	InvalidRootParent,
	MissingNodeReference,
	MultipleParents,
	InvalidSegmentOrder,
	CycleDetected,
	DisconnectedNode,
	InvalidAttachment,
	InvalidBud,
	InvalidGrowthTip,
	RadiusConservationViolation
};

class BranchGraphValidationIssue
{
public:
	BranchGraphValidationIssue(BranchGraphDiagnosticCode code,
	                           std::string diagnostic)
		: code_(code), diagnostic_(std::move(diagnostic))
	{
	}

	BranchGraphDiagnosticCode code() const { return code_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	BranchGraphDiagnosticCode code_ = BranchGraphDiagnosticCode::MissingRoot;
	std::string diagnostic_;
};

class BranchGraphValidationReport
{
public:
	void addIssue(BranchGraphDiagnosticCode code, std::string diagnostic)
	{
		issues_.emplace_back(code, std::move(diagnostic));
	}

	bool succeeded() const { return issues_.empty(); }
	const std::vector<BranchGraphValidationIssue> &issues() const
	{
		return issues_;
	}

private:
	std::vector<BranchGraphValidationIssue> issues_;
};

