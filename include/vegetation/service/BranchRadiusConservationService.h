#pragma once

#include "vegetation/model/BranchGraphValidationReport.h"

class BranchGraph;

class BranchRadiusConservationService
{
public:
	BranchGraphValidationReport validate(
		const BranchGraph &graph,
		float exponent_gamma,
		float relative_tolerance) const;
};

