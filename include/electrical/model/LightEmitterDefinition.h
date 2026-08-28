#pragma once

#include "lighting/model/LightId.h"
#include "lighting/model/PhotometricEmission.h"
#include "lighting/model/SceneLightType.h"

#include <glm/glm.hpp>

class LightEmitterDefinition
{
public:
	LightId scene_light_id;
	glm::mat4 local_transform{1.0f};
	SceneLightType type = SceneLightType::Spot;
	PhotometricEmission emission;
	float range = 8.0f;
	float inner_cone_radians = 0.35f;
	float outer_cone_radians = 0.55f;
};
