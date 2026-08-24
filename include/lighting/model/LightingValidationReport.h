#pragma once

#include "lighting/model/LightingValidationIssue.h"

#include <string>
#include <utility>
#include <vector>

class LightingValidationReport
{
public:
	void addIssue(LightingValidationIssue issue) { issues_.push_back(std::move(issue)); }
	bool isValid() const { return issues_.empty(); }
	const std::vector<LightingValidationIssue> &issues() const { return issues_; }
	std::string firstDiagnostic() const
	{
		return issues_.empty() ? std::string() : issues_.front().message();
	}

private:
	std::vector<LightingValidationIssue> issues_;
};
