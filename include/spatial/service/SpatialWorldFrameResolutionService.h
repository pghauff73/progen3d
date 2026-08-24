#pragma once

#include "spatial/graph/SpatialContainmentTree.h"
#include "spatial/graph/SpatialObjectRegistry.h"
#include "spatial/model/SpatialModelValidationReport.h"

#include <optional>

class SpatialWorldFrameResolutionService {
public:
	std::optional<SpatialObjectRegistry> resolve(
		const SpatialObjectRegistry &objects,
		const SpatialContainmentTree &containment_tree,
		SpatialModelValidationReport *validation_report) const;
};
