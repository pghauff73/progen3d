#pragma once

#include "vegetation/model/PlantTopologyDecomposition.h"

class BranchGraph;

class PlantTopologyDecompositionService
{
public:
	PlantTopologyDecomposition decompose(const BranchGraph &graph) const;
};
