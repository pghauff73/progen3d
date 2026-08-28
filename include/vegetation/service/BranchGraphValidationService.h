#pragma once

#include "vegetation/model/BranchGraphValidationReport.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <string>
#include <vector>

class BranchGraph;

class BranchGraphValidationService
{
public:
	explicit BranchGraphValidationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	BranchGraphValidationReport validate(const BranchGraph &graph) const;
	std::vector<std::string> deterministicTraversal(
		const BranchGraph &graph,
		std::string *diagnostic = nullptr) const;

private:
	VegetationComplexityLimits complexity_limits_;
};

