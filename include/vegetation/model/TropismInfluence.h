#pragma once

#include "vegetation/model/TropismType.h"

#include <glm/glm.hpp>

class TropismInfluence
{
public:
	TropismInfluence(TropismType type, glm::vec3 direction, float weight)
		: type_(type), direction_(direction), weight_(weight)
	{
	}

	TropismType type() const { return type_; }
	const glm::vec3 &direction() const { return direction_; }
	float weight() const { return weight_; }

private:
	TropismType type_ = TropismType::Custom;
	glm::vec3 direction_{0.0f};
	float weight_ = 0.0f;
};

