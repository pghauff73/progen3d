#pragma once

#include "geometry/model/BinarySilhouette.h"
#include "geometry/model/Profile2D.h"
#include "geometry/service/ThreeViewProjectionService.h"

#include <glm/glm.hpp>

#include <vector>

class ProfileSectionProjectionService
{
public:
	BinarySilhouette project(
		const Profile2D &profile,
		const glm::vec2 &center = glm::vec2(0.0f),
		const glm::vec2 &scale = glm::vec2(1.0f),
		float rotation_degrees = 0.0f,
		const ThreeViewProjectionConfiguration &configuration = {}) const;

	BinarySilhouette project(
		const std::vector<Profile2D> &profiles,
		const glm::vec2 &center = glm::vec2(0.0f),
		const glm::vec2 &scale = glm::vec2(1.0f),
		float rotation_degrees = 0.0f,
		const ThreeViewProjectionConfiguration &configuration = {}) const;
};
