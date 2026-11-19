#pragma once

#include "physics/model/CollisionContact.h"
class ScenePrimitiveInstance;

// Lateral contact of parallel shaft-mounted extrusions with solid polygon profiles.
class ExtrudedProfileContact {
public:
	static bool accepts(const ScenePrimitiveInstance &body);
	static CollisionContact detect(const ScenePrimitiveInstance &first, const ScenePrimitiveInstance &second);
};
