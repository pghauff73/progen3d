#pragma once

#include "spatial/model/SpatialModelValidationIssue.h"

#include <string>
#include <utility>
#include <vector>

class SpatialModelValidationReport {
public:
	void addIssue(SpatialModelValidationIssue issue)
	{
		issues_.push_back(std::move(issue));
	}

	void append(const SpatialModelValidationReport &other)
	{
		issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
	}

	bool isValid() const { return issues_.empty(); }
	const std::vector<SpatialModelValidationIssue> &issues() const { return issues_; }

	std::string firstDiagnostic() const
	{
		return issues_.empty() ? std::string() : issues_.front().message();
	}

private:
	std::vector<SpatialModelValidationIssue> issues_;
};
