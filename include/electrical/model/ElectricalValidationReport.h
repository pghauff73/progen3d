#pragma once

#include "electrical/model/ElectricalValidationIssue.h"

#include <string>
#include <utility>
#include <vector>

class ElectricalValidationReport
{
public:
	void addIssue(ElectricalValidationIssue issue) { issues_.push_back(std::move(issue)); }
	bool isValid() const { return issues_.empty(); }
	const std::vector<ElectricalValidationIssue> &issues() const { return issues_; }
	std::string firstDiagnostic() const
	{
		return issues_.empty() ? std::string() : issues_.front().message();
	}

private:
	std::vector<ElectricalValidationIssue> issues_;
};
