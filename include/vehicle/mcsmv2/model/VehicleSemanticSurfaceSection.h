#pragma once

#include "vehicle/mcsmv2/model/VehicleSectionInterpolationRelationship.h"

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class VehicleSemanticSurfaceSection
{
public:
	VehicleSemanticSurfaceSection(
		VehicleSectionInterpolationRelationship section_relationship,
		std::vector<glm::dvec3> source_ring_points)
		: section_relationship_(std::move(section_relationship)),
		  source_ring_points_(std::move(source_ring_points))
	{
	}

	const VehicleSectionInterpolationRelationship &sectionRelationship() const
	{
		return section_relationship_;
	}
	const std::vector<glm::dvec3> &sourceRingPoints() const
	{
		return source_ring_points_;
	}

private:
	VehicleSectionInterpolationRelationship section_relationship_{0.0, 0u, 0u, 0.0};
	std::vector<glm::dvec3> source_ring_points_;
};
