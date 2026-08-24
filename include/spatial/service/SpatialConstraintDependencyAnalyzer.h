#pragma once

#include "spatial/graph/SpatialConstraintGraph.h"
#include "spatial/graph/SpatialObjectRegistry.h"
#include "spatial/model/SpatialConstraintDependencyReport.h"
#include "spatial/model/SpatialModelSafetyLimits.h"

class SpatialConstraintDependencyAnalyzer {
public:
	SpatialConstraintDependencyReport analyze(
		const SpatialConstraintGraph &constraint_graph,
		const SpatialObjectRegistry &objects,
		const SpatialModelSafetyLimits &limits = SpatialModelSafetyLimits()) const;
};
