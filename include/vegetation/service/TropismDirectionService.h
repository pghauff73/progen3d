#pragma once

#include "vegetation/model/TropismInfluence.h"
#include "vegetation/model/TropismResolution.h"

#include <glm/glm.hpp>

#include <vector>

class TropismDirectionService
{
public:
	TropismResolution resolve(
		glm::vec3 current_direction,
		const std::vector<TropismInfluence> &influences) const;
};

