#pragma once

#include <glm/glm.hpp>

class PhotometricColorService
{
public:
	glm::vec3 kelvinToLinearRgb(float kelvin) const;
};
