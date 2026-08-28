#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticGeometry.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"
#include "vehicle/mcsmv2/model/McsMv22SemanticKinematicBindings.h"

class McsMv22SemanticKinematicBindingService
{
public:
	McsMv22SemanticKinematicBindings bind(
		const McsMv21SemanticGeometry &semantic_geometry,
		const McsMv22KinematicVariantDefinition &kinematic_definition) const;
};
