#include "vegetation/service/SpaceColonizationSpecificationValidationService.h"

#include <glm/geometric.hpp>

#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

} // namespace

bool SpaceColonizationSpecificationValidationService::validate(
	const SpaceColonizationSpecification &specification,
	std::string *diagnostic) const
{
	if (!std::isfinite(specification.influenceRadius()) ||
	    !std::isfinite(specification.killRadius()) ||
	    !std::isfinite(specification.stepLength()) ||
	    specification.influenceRadius() <= 0.0f ||
	    specification.killRadius() <= 0.0f ||
	    specification.killRadius() >= specification.influenceRadius() ||
	    specification.stepLength() <= 0.0f ||
	    specification.maximumIterations() == 0u ||
	    specification.maximumIterations() >
		    complexity_limits_.maximumGrowthIterations() ||
	    specification.attractionPointCount() == 0u ||
	    specification.attractionPointCount() >
		    complexity_limits_.maximumAttractionPoints() ||
	    !std::isfinite(specification.radiusDecay()) ||
	    specification.radiusDecay() <= 0.0f ||
	    specification.radiusDecay() > 1.0f ||
	    !std::isfinite(specification.minimumRadius()) ||
	    specification.minimumRadius() <= 0.0f ||
	    !std::isfinite(specification.radiusConservationExponent()) ||
	    specification.radiusConservationExponent() <= 0.0f ||
	    !finite(specification.tropismDirection()) ||
	    !std::isfinite(specification.tropismWeight()) ||
	    specification.tropismWeight() < 0.0f ||
	    (specification.tropismWeight() > 0.0f &&
	     glm::length(specification.tropismDirection()) <= 1.0e-6f)) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Space colonization requires finite radii and step values, kill radius "
				"below influence radius, bounded counts, positive radius settings, and "
				"a non-zero weighted tropism direction.";
		}
		return false;
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return true;
}
