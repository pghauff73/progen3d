#include "vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

McsMv22ClosureKinematicEvaluationService::
	McsMv22ClosureKinematicEvaluationService(
		VehicleClosureMotionEvaluationService motion_service)
	: motion_service_(std::move(motion_service))
{
}

VehicleClosureSweepSet
McsMv22ClosureKinematicEvaluationService::createClosureSweepSet(
	const McsMv22KinematicVariantDefinition &variant) const
{
	static constexpr std::array<double, 7> states =
		{0.10, 0.25, 0.40, 0.55, 0.70, 0.85, 1.00};
	std::vector<VehicleClosureSweep> sweeps;
	sweeps.reserve(variant.hinges().size());
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		VehicleClosureSurfaceBinding binding = createClosureBinding(variant, hinge);
		std::vector<VehicleClosurePoseSample> samples;
		samples.reserve(states.size());
		for (double state : states) {
			samples.push_back(
				motion_service_.evaluateClosurePose(binding.joint(), state));
		}
		sweeps.emplace_back(std::move(binding), std::move(samples));
	}
	return VehicleClosureSweepSet(std::move(sweeps));
}

VehicleGlassMotionSet
McsMv22ClosureKinematicEvaluationService::createGlassMotionSet(
	const McsMv22KinematicVariantDefinition &variant,
	double parent_closure_state) const
{
	static constexpr std::array<double, 6> states =
		{0.0, 0.2, 0.4, 0.6, 0.8, 1.0};
	std::vector<VehicleGlassMotion> motions;
	motions.reserve(variant.glassSystems().size());
	for (const McsMv22HelicalGlassDefinition &glass : variant.glassSystems()) {
		VehicleGlassSurfaceBinding binding = createGlassBinding(variant, glass);
		const McsMv22ClosureHingeDefinition &parent_hinge =
			findParentHinge(variant, binding.parentClosureIdentifier());
		const VehicleClosureSurfaceBinding parent_binding =
			createClosureBinding(variant, parent_hinge);
		const glm::dmat4 parent_transform = motion_service_
			.evaluateClosurePose(parent_binding.joint(), parent_closure_state)
			.sourceTransform();
		std::vector<VehicleGlassPoseSample> samples;
		samples.reserve(states.size());
		for (double state : states) {
			samples.push_back(motion_service_.evaluateGlassPose(
				binding.motion(), state, parent_transform));
		}
		motions.emplace_back(std::move(binding), std::move(samples));
	}
	return VehicleGlassMotionSet(std::move(motions));
}

VehicleJointGraph McsMv22ClosureKinematicEvaluationService::createJointGraph(
	const McsMv22KinematicVariantDefinition &variant) const
{
	VehicleJointGraph graph;
	std::string diagnostic;
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		if (!graph.addJoint(
				VehicleJoint(
					hinge.identifier(), VehicleJointType::Revolute,
					hinge.closureIdentifier(), "closure_hinge", "vehicle_body",
					hinge.identifier(), glm::vec3(hinge.sourceAxis()), 0.0f,
					static_cast<float>(hinge.maximumAngleDegrees()), 0.0f),
				&diagnostic)) {
			throw std::runtime_error(diagnostic);
		}
	}
	for (const McsMv22HelicalGlassDefinition &glass : variant.glassSystems()) {
		if (!graph.addJoint(
				VehicleJoint(
					"joint_" + glass.identifier(), VehicleJointType::Prismatic,
					glass.identifier(), "glass_carrier",
					glass.parentClosureIdentifier(), "door_glass_guide",
					glm::vec3(0.0f, 0.0f, -1.0f), 0.0f,
					static_cast<float>(glass.travelMetres()), 0.0f),
				&diagnostic)) {
			throw std::runtime_error(diagnostic);
		}
	}
	return graph;
}

VehicleClosureSurfaceBinding
McsMv22ClosureKinematicEvaluationService::createClosureBinding(
	const McsMv22KinematicVariantDefinition &variant,
	const McsMv22ClosureHingeDefinition &hinge) const
{
	for (const McsMv22SurfaceOwnerDefinition &owner : variant.panelOwners()) {
		if (owner.identifier() == hinge.closureIdentifier() && owner.isClosure()) {
			return VehicleClosureSurfaceBinding(
				owner.identifier(), hinge.closureIdentifier(), owner.faceCount(),
				VehicleRisingRevoluteJoint(
					hinge.identifier(), hinge.sourcePoint(), hinge.sourceAxis(),
					hinge.maximumAngleDegrees(), hinge.direction(),
					hinge.riseMetres(), hinge.sourceTranslationAxis(),
					hinge.jointType()));
		}
	}
	throw std::runtime_error(
		"MCSMv2.2 closure has no exact semantic surface owner: " +
		hinge.closureIdentifier());
}

VehicleGlassSurfaceBinding
McsMv22ClosureKinematicEvaluationService::createGlassBinding(
	const McsMv22KinematicVariantDefinition &variant,
	const McsMv22HelicalGlassDefinition &glass) const
{
	findParentHinge(variant, glass.parentClosureIdentifier());
	for (const McsMv22SurfaceOwnerDefinition &owner : variant.apertureOwners()) {
		if (owner.identifier() == glass.identifier()) {
			return VehicleGlassSurfaceBinding(
				owner.identifier(), glass.parentClosureIdentifier(), owner.faceCount(),
				VehicleHelicalGlassMotion(
					glass.identifier(), glass.side(), glass.travelMetres(),
					glass.inwardMetres(), glass.longitudinalMetres(),
					glass.rotationDegrees(), glass.sourcePivot()));
		}
	}
	throw std::runtime_error(
		"MCSMv2.2 glass has no exact semantic aperture owner: " +
		glass.identifier());
}

const McsMv22ClosureHingeDefinition &
McsMv22ClosureKinematicEvaluationService::findParentHinge(
	const McsMv22KinematicVariantDefinition &variant,
	const std::string &closure_identifier) const
{
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		if (hinge.closureIdentifier() == closure_identifier) return hinge;
	}
	throw std::runtime_error(
		"MCSMv2.2 glass parent closure has no hinge: " + closure_identifier);
}
