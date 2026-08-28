#pragma once

#include "Mesh.h"
#include "vehicle/mcsmv2/model/SemanticImplicitCorrespondenceReport.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

class SemanticImplicitAgreementEvaluationService
{
public:
	SemanticImplicitCorrespondenceReport evaluate(
		const Mesh &registered_semantic_surface,
		const std::vector<glm::dvec2> &registered_vertex_surface_coordinates,
		const Mesh &implicit_scaffold_surface,
		const SemanticImplicitCorrespondenceReport &accepted_correspondence,
		std::size_t maximum_samples = 6000u) const;
};
