#include "vehicle/service/VehicleClosureMotionEvaluationService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

double normalizedState(double state)
{
	return std::clamp(state, 0.0, 1.0);
}

glm::dmat4 rotationAboutPivot(
	const glm::dvec3 &pivot,
	const glm::dvec3 &axis,
	double angle_degrees)
{
	const double axis_length = glm::length(axis);
	if (!std::isfinite(axis_length) || axis_length <= 1.0e-12) {
		throw std::invalid_argument("Closure rotation axis must be finite and nonzero.");
	}
	return glm::translate(glm::dmat4(1.0), pivot) *
	       glm::rotate(
		       glm::dmat4(1.0), glm::radians(angle_degrees), axis / axis_length) *
	       glm::translate(glm::dmat4(1.0), -pivot);
}

} // namespace

VehicleClosurePoseSample
VehicleClosureMotionEvaluationService::evaluateClosurePose(
	const VehicleRisingRevoluteJoint &joint,
	double normalized_state) const
{
	const double state = normalizedState(normalized_state);
	const double angle_degrees =
		joint.direction() * joint.maximumAngleDegrees() * state;
	const double rise_metres =
		joint.maximumRiseMetres() * std::sin(0.5 * glm::pi<double>() * state);
	const glm::dmat4 rotation = rotationAboutPivot(
		joint.sourcePivot(), joint.sourceAxis(), angle_degrees);
	const glm::dmat4 translation = glm::translate(
		glm::dmat4(1.0), joint.sourceTranslationAxis() * rise_metres);
	return VehicleClosurePoseSample(
		state, angle_degrees, rise_metres, translation * rotation);
}

glm::dmat4 VehicleClosureMotionEvaluationService::evaluateGlassLocalTransform(
	const VehicleHelicalGlassMotion &motion,
	double normalized_state) const
{
	const double state = normalizedState(normalized_state);
	const double side_sign = motion.side() == "left" ? -1.0 : 1.0;
	if (motion.side() != "left" && motion.side() != "right") {
		throw std::invalid_argument("Glass motion side must be left or right.");
	}
	const glm::dvec3 offset(
		motion.longitudinalMetres() * state,
		-side_sign * motion.inwardMetres() * state,
		-motion.travelMetres() * state);
	return glm::translate(glm::dmat4(1.0), offset) *
	       rotationAboutPivot(
		       motion.sourcePivot(), glm::dvec3(1.0, 0.0, 0.0),
		       side_sign * motion.rotationDegrees() * state);
}

VehicleGlassPoseSample
VehicleClosureMotionEvaluationService::evaluateGlassPose(
	const VehicleHelicalGlassMotion &motion,
	double normalized_state,
	const glm::dmat4 &parent_closure_source_transform) const
{
	const double state = normalizedState(normalized_state);
	const glm::dmat4 local_transform =
		evaluateGlassLocalTransform(motion, state);
	return VehicleGlassPoseSample(
		state, motion.travelMetres() * state,
		motion.rotationDegrees() * state, local_transform,
		parent_closure_source_transform * local_transform);
}
