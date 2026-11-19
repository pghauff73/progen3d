#pragma once

#include "geometry/model/CubeFace.h"

#include <glm/vec3.hpp>
#include <optional>

class ScenePrimitiveInstance;

bool hasAffineCubeFaces(const ScenePrimitiveInstance &primitive);
std::optional<CubeFace> cubeFaceAtWorldPoint(const ScenePrimitiveInstance &primitive,
                                            const glm::vec3 &world_point);
std::array<glm::vec3, 4> cubeFaceWorldCorners(const ScenePrimitiveInstance &primitive,
                                             CubeFace face);
