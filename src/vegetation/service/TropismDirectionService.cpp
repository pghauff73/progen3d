#include "vegetation/service/TropismDirectionService.h"

#include <glm/geometric.hpp>

#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

} // namespace

TropismResolution TropismDirectionService::resolve(
	glm::vec3 current_direction,
	const std::vector<TropismInfluence> &influences) const
{
	if (!finite(current_direction) || glm::length(current_direction) <= 1.0e-6f) {
		return TropismResolution::failed(
			"Tropism requires a finite non-zero current direction.");
	}
	current_direction = glm::normalize(current_direction);
	glm::vec3 environmental_vector(0.0f);
	for (const TropismInfluence &influence : influences) {
		if (!finite(influence.direction()) ||
		    !std::isfinite(influence.weight()) || influence.weight() < 0.0f) {
			return TropismResolution::failed(
				"Tropism influences require finite directions and non-negative weights.");
		}
		if (influence.weight() == 0.0f) continue;
		if (glm::length(influence.direction()) <= 1.0e-6f) {
			return TropismResolution::failed(
				"A weighted tropism influence cannot have a zero direction.");
		}
		environmental_vector +=
			influence.weight() * glm::normalize(influence.direction());
	}
	const glm::vec3 combined = current_direction + environmental_vector;
	if (glm::length(combined) <= 1.0e-6f) {
		return TropismResolution::failed(
			"Tropism influences cancel the current growth direction.");
	}
	return TropismResolution::succeeded(
		glm::normalize(combined), environmental_vector);
}

