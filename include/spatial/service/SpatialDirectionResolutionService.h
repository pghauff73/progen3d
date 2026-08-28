#pragma once

#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialDirection.h"

#include <optional>
#include <string>

#include <glm/glm.hpp>

class SpatialDirectionResolutionService {
public:
	std::optional<glm::vec3> resolve(
		const SpatialDirection &direction,
		const SpatialBuildingObject &moving_object,
		const SpatialInterface &moving_interface,
		const SpatialBuildingObject &target_object,
		const SpatialInterface &target_interface,
		const SpatialBuildingModel &model,
		std::string *diagnostic) const;
};
