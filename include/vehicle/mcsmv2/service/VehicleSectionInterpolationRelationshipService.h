#pragma once

#include "vehicle/mcsmv2/model/VehicleSectionInterpolationRelationship.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSectionField.h"

class VehicleSectionInterpolationRelationshipService
{
public:
	VehicleSectionInterpolationRelationship resolve(
		const VehicleSemanticSectionField &section_field,
		double source_x) const;
};
