#pragma once

#include "physics/model/SurfaceContactState.h"

class ScenePrimitiveInstance;
struct CollisionGeometry;

class SurfaceContactSolver {
public:
	static bool initializeInertia(ScenePrimitiveInstance &body, const CollisionGeometry &geometry);
	static void advance(ScenePrimitiveInstance &first, ScenePrimitiveInstance &second,
	                    const glm::dvec3 &first_center, const glm::dvec3 &second_center,
	                    const glm::dvec3 &point, const glm::dvec3 &normal, double depth,
	                    double seconds, SurfaceContactPatch &patch);
	static void release(ScenePrimitiveInstance &first, ScenePrimitiveInstance &second,
	                    const SurfaceContactPatch &patch);
};
