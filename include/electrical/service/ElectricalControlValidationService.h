#pragma once

#include "electrical/model/ElectricalControlGraph.h"
#include "electrical/model/ElectricalValidationReport.h"

class ElectricalControlValidationService
{
public:
	ElectricalValidationReport validate(const ElectricalControlGraph &graph) const;
};
