#pragma once

#include "vegetation/model/OrganArrayPlacement.h"
#include "vegetation/model/PlantOrganArraySpecification.h"
#include "vegetation/model/SurfaceAttachmentPoint.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class OrganArrayPlacementService
{
public:
	std::vector<OrganArrayPlacement> placeAlongPath(
		const PlantOrganArraySpecification &specification,
		const std::vector<glm::vec3> &path_points,
		std::uint64_t deterministic_seed,
		std::string *diagnostic = nullptr) const;

	std::vector<OrganArrayPlacement> placeAroundFlowerHead(
		const PlantOrganArraySpecification &specification,
		glm::vec3 origin,
		glm::vec3 axis,
		std::uint64_t deterministic_seed,
		std::string *diagnostic = nullptr) const;

	std::vector<OrganArrayPlacement> placeOnSurface(
		const PlantOrganArraySpecification &specification,
		const std::vector<SurfaceAttachmentPoint> &surface_samples,
		std::uint64_t deterministic_seed,
		std::string *diagnostic = nullptr) const;
};
