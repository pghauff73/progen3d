#pragma once

#include "vehicle/model/VehicleClassASurface.h"
#include "vehicle/model/VehicleMvp25Evidence.h"

#include <glm/glm.hpp>

#include <optional>

class VehicleProjectionService
{
public:
	std::optional<glm::vec2> projectPoint(
		const VehicleCameraModel &camera,
		const glm::vec3 &world_point) const;

	float calculateLandmarkError(
		const VehicleLandmark &landmark,
		const VehicleObservationSet &observations) const;

	float calculateSilhouetteError(
		const SilhouetteConstraint &constraint,
		const VehicleObservationSet &observations) const;

	float calculateCharacterLineError(
		const AutomotiveCharacterCurve &curve,
		const VehicleObservationSet &observations) const;
};
