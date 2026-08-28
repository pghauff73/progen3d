#pragma once

#include "vegetation/model/PhyllotaxisSpecification.h"
#include "vegetation/model/VegetationOrganPlacement.h"
#include "vegetation/model/WhorlSpecification.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <vector>

class OrganPlacementService
{
public:
	std::vector<VegetationOrganPlacement> placeAlongPath(
		const std::string &identifier_prefix,
		const std::vector<glm::vec3> &path_points,
		std::size_t node_count,
		const PhyllotaxisSpecification &specification,
		std::string *diagnostic = nullptr) const;

	std::vector<VegetationOrganPlacement> placeWhorl(
		const std::string &identifier_prefix,
		glm::vec3 origin,
		glm::vec3 axis,
		const WhorlSpecification &specification,
		std::string *diagnostic = nullptr) const;

	std::vector<VegetationOrganPlacement> placeAtNode(
		const std::string &identifier_prefix,
		std::size_t node_index,
		glm::vec3 origin,
		glm::vec3 axis,
		const PhyllotaxisSpecification &specification,
		std::string *diagnostic = nullptr) const;
};
