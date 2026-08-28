#pragma once

#include "geometry/model/ConvexHullMeshGenerationResult.h"

#include <glm/glm.hpp>

#include <vector>

class ConvexHullMeshGenerationService
{
public:
	ConvexHullMeshGenerationResult generateConservativeEnvelope(
		const std::vector<glm::dvec3> &source_points,
		double quantization_metres = 1.0e-6) const;
};
