#include "vehicle/mcsmv2/service/McsMv22KinematicPoseEvaluationService.h"

#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string wheelIdentifier(VehicleCornerLocation corner)
{
	switch (corner) {
	case VehicleCornerLocation::FrontLeft: return "front_left";
	case VehicleCornerLocation::FrontRight: return "front_right";
	case VehicleCornerLocation::RearLeft: return "rear_left";
	case VehicleCornerLocation::RearRight: return "rear_right";
	}
	throw std::runtime_error("Unsupported vehicle corner.");
}

double requiredState(
	const std::map<std::string, double> &states,
	const std::string &identifier)
{
	const auto state = states.find(identifier);
	if (state == states.end()) {
		throw std::invalid_argument(
			"MCSMv2.2 selected state is missing '" + identifier + "'.");
	}
	if (!std::isfinite(state->second) || state->second < 0.0 || state->second > 1.0) {
		throw std::invalid_argument(
			"MCSMv2.2 selected normalized state is outside [0, 1]: " + identifier);
	}
	return state->second;
}

template <typename StateValue>
void requireExactStateIdentifiers(
	const std::map<std::string, StateValue> &states,
	const std::set<std::string> &expected_identifiers,
	const std::string &state_category)
{
	if (states.size() != expected_identifiers.size()) {
		throw std::invalid_argument(
			"MCSMv2.2 " + state_category +
			" state count does not match the selected variant.");
	}
	for (const auto &state : states) {
		if (expected_identifiers.count(state.first) == 0u) {
			throw std::invalid_argument(
				"MCSMv2.2 " + state_category +
				" state contains an unknown identifier: " + state.first);
		}
	}
}

VehicleRisingRevoluteJoint createJoint(
	const McsMv22ClosureHingeDefinition &hinge)
{
	return VehicleRisingRevoluteJoint(
		hinge.identifier(), hinge.sourcePoint(), hinge.sourceAxis(),
		hinge.maximumAngleDegrees(), hinge.direction(), hinge.riseMetres(),
		hinge.sourceTranslationAxis(), hinge.jointType());
}

VehicleHelicalGlassMotion createGlassMotion(
	const McsMv22HelicalGlassDefinition &glass)
{
	return VehicleHelicalGlassMotion(
		glass.identifier(), glass.side(), glass.travelMetres(),
		glass.inwardMetres(), glass.longitudinalMetres(),
		glass.rotationDegrees(), glass.sourcePivot());
}

} // namespace

McsMv22KinematicState McsMv22KinematicPoseEvaluationService::createNeutralState(
	const McsMv22KinematicVariantDefinition &variant) const
{
	std::map<std::string, McsMv22WheelControlState> wheels;
	for (const VehicleSuspensionCornerKinematicModel &corner :
	     mcsm_suspension_service_.createCornerModels(variant)) {
		wheels.emplace(wheelIdentifier(corner.corner()), McsMv22WheelControlState(0.0, 0.0));
	}
	std::map<std::string, double> closures;
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		closures.emplace(hinge.closureIdentifier(), 0.0);
	}
	std::map<std::string, double> glass;
	for (const McsMv22HelicalGlassDefinition &system : variant.glassSystems()) {
		glass.emplace(system.identifier(), 0.0);
	}
	return McsMv22KinematicState(
		std::move(wheels), std::move(closures), std::move(glass));
}

McsMv22EvaluatedKinematicState
McsMv22KinematicPoseEvaluationService::evaluate(
	const McsMv22KinematicVariantDefinition &variant,
	const McsMv220SourceRelease &source_release,
	const McsMv22KinematicState &state) const
{
	const std::vector<VehicleSuspensionCornerKinematicModel> corner_models =
		mcsm_suspension_service_.createCornerModels(variant);
	std::set<std::string> expected_wheel_identifiers;
	for (const VehicleSuspensionCornerKinematicModel &corner : corner_models) {
		expected_wheel_identifiers.insert(wheelIdentifier(corner.corner()));
	}
	std::set<std::string> expected_closure_identifiers;
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		expected_closure_identifiers.insert(hinge.closureIdentifier());
	}
	std::set<std::string> expected_glass_identifiers;
	for (const McsMv22HelicalGlassDefinition &glass : variant.glassSystems()) {
		expected_glass_identifiers.insert(glass.identifier());
	}
	requireExactStateIdentifiers(
		state.wheelStates(), expected_wheel_identifiers, "wheel");
	requireExactStateIdentifiers(
		state.closureStates(), expected_closure_identifiers, "closure");
	requireExactStateIdentifiers(
		state.glassStates(), expected_glass_identifiers, "glass");

	std::map<std::string, McsMv22EvaluatedWheelState> wheels;
	for (const VehicleSuspensionCornerKinematicModel &corner : corner_models) {
		const std::string identifier = wheelIdentifier(corner.corner());
		const auto selected = state.wheelStates().find(identifier);
		if (selected == state.wheelStates().end()) {
			throw std::invalid_argument(
				"MCSMv2.2 selected state is missing wheel '" + identifier + "'.");
		}
		const double steering = selected->second.steeringDegrees();
		const double travel = selected->second.suspensionTravelMetres();
		if (!std::isfinite(steering) || !std::isfinite(travel) ||
		    steering < corner.minimumSteeringDegrees() ||
		    steering > corner.maximumSteeringDegrees() ||
		    travel < corner.minimumTravelMetres() ||
		    travel > corner.maximumTravelMetres()) {
			throw std::invalid_argument(
				"MCSMv2.2 wheel state is outside its declared interval: " + identifier);
		}
		VehicleWheelPoseEvaluation source_pose =
			suspension_service_.evaluateWheelPose(
				corner.corner(), corner.nominalSourceHubCentre(),
				corner.sourceTrackWidthMetres(), corner.wheelRadiusMetres(), steering,
				travel, corner.solutionPolicy());
		wheels.emplace(
			identifier,
			McsMv22EvaluatedWheelState(
				source_pose,
				frame_service_.transformRigidBodyPose(
					source_pose.sourceTransform(), source_release.toProgen3dMatrix())));
	}

	std::map<std::string, glm::dmat4> source_closure_transforms;
	std::map<std::string, glm::dmat4> target_closure_transforms;
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		const double selected_state =
			requiredState(state.closureStates(), hinge.closureIdentifier());
		const glm::dmat4 source_transform = closure_service_
			.evaluateClosurePose(createJoint(hinge), selected_state)
			.sourceTransform();
		source_closure_transforms.emplace(hinge.closureIdentifier(), source_transform);
		target_closure_transforms.emplace(
			hinge.closureIdentifier(),
			frame_service_.transformRigidBodyPose(
				source_transform, source_release.toProgen3dMatrix()));
	}

	std::map<std::string, glm::dmat4> target_glass_transforms;
	for (const McsMv22HelicalGlassDefinition &glass : variant.glassSystems()) {
		const double selected_state = requiredState(state.glassStates(), glass.identifier());
		const auto parent_transform =
			source_closure_transforms.find(glass.parentClosureIdentifier());
		if (parent_transform == source_closure_transforms.end()) {
			throw std::invalid_argument(
				"MCSMv2.2 glass parent closure was not evaluated: " +
				glass.parentClosureIdentifier());
		}
		const glm::dmat4 source_transform = closure_service_
			.evaluateGlassPose(
				createGlassMotion(glass), selected_state, parent_transform->second)
			.worldSourceTransform();
		target_glass_transforms.emplace(
			glass.identifier(),
			frame_service_.transformRigidBodyPose(
				source_transform, source_release.toProgen3dMatrix()));
	}
	return McsMv22EvaluatedKinematicState(
		std::move(wheels), std::move(target_closure_transforms),
		std::move(target_glass_transforms));
}
