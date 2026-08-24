#pragma once

#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/model/SpatialModelSafetyLimits.h"
#include "spatial/model/SpatialModelValidationReport.h"
#include "spatial/relationship/SpatialContainmentRelationship.h"

#include <vector>

class SpatialContainmentValidationService {
public:
	SpatialModelValidationReport validate(
		const std::vector<SpatialBuildingObject> &objects,
		const SpatialObjectId &root_object_id,
		const std::vector<SpatialContainmentRelationship> &relationships,
		const SpatialModelSafetyLimits &limits = SpatialModelSafetyLimits()) const;
};
