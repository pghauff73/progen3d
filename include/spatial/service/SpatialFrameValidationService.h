#pragma once

#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialModelValidationReport.h"

#include <string>

class SpatialFrameValidationService {
public:
	SpatialModelValidationReport validateObjectFrame(
		const SpatialBuildingObject &object) const;

	bool normalizeInterfaceFrame(const SpatialInterfaceFrame &input_frame,
	                             SpatialInterfaceFrame *normalized_frame,
	                             std::string *diagnostic) const;
};
