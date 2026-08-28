#pragma once

#include "geometry/model/SweepProfileShapeSpecification.h"

class PanelCutShapeSpecification : public SweepProfileShapeSpecification
{
public:
	PanelCutShapeSpecification(
		Profile2D profile,
		std::vector<glm::vec3> path_points,
		glm::vec3 up_hint,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: SweepProfileShapeSpecification(
			std::move(profile), std::move(path_points), up_hint, cap_policy,
			std::move(key), std::move(canonical_text), detail_level,
			ShapeFamily::PanelCut)
	{
	}
};
