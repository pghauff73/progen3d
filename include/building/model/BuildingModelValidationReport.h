#pragma once

#include "building/model/BuildingModelValidationIssue.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class BuildingModelValidationReport {
public:
	void addIssue(BuildingModelValidationIssue issue)
	{
		issues_.push_back(std::move(issue));
	}

	void append(const BuildingModelValidationReport &other)
	{
		issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
	}

	bool isValid() const { return issues_.empty(); }
	const std::vector<BuildingModelValidationIssue> &issues() const { return issues_; }
	void enforceMaximumIssueCount(std::size_t maximum_issue_count)
	{
		if (issues_.size() <= maximum_issue_count) return;
		if (maximum_issue_count == 0) {
			issues_.clear();
			issues_.push_back(BuildingModelValidationIssue(
				BuildingModelValidationCode::SafetyCeilingExceeded,
				"Building model diagnostics exceed the configured safety ceiling."));
			return;
		}
		issues_.erase(
			issues_.begin() + static_cast<std::ptrdiff_t>(maximum_issue_count),
			issues_.end());
		issues_.back() = BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building model diagnostics exceed the configured safety ceiling.");
	}

	std::string firstDiagnostic() const
	{
		return issues_.empty() ? std::string() : issues_.front().message();
	}

private:
	std::vector<BuildingModelValidationIssue> issues_;
};
