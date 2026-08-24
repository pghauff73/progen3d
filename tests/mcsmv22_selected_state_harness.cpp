#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicPoseEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

bool matricesMatch(
	const glm::dmat4 &left,
	const glm::dmat4 &right,
	double tolerance = 1.0e-12)
{
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			if (std::abs(left[column][row] - right[column][row]) > tolerance) {
				return false;
			}
		}
	}
	return true;
}

bool isIdentity(const glm::dmat4 &matrix, double tolerance = 1.0e-12)
{
	return matricesMatch(matrix, glm::dmat4(1.0), tolerance);
}

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

McsMv22KinematicState createFullState(
	const McsMv22KinematicVariantDefinition &variant)
{
	std::map<std::string, McsMv22WheelControlState> wheels;
	for (const VehicleSuspensionCornerKinematicModel &corner :
	     McsMv22SuspensionKinematicEvaluationService().createCornerModels(variant)) {
		wheels.emplace(
			wheelIdentifier(corner.corner()),
			McsMv22WheelControlState(
				corner.maximumSteeringDegrees(), corner.maximumTravelMetres()));
	}
	std::map<std::string, double> closures;
	for (const McsMv22ClosureHingeDefinition &hinge : variant.hinges()) {
		closures.emplace(hinge.closureIdentifier(), 1.0);
	}
	std::map<std::string, double> glass;
	for (const McsMv22HelicalGlassDefinition &system : variant.glassSystems()) {
		glass.emplace(system.identifier(), 1.0);
	}
	return McsMv22KinematicState(
		std::move(wheels), std::move(closures), std::move(glass));
}

template <typename Callable>
bool rejectsInvalidState(Callable &&callable)
{
	try {
		callable();
	} catch (const std::invalid_argument &) {
		return true;
	}
	return false;
}

} // namespace

int main()
{
	const McsMv22KinematicFamilyDefinition family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const McsMv22KinematicPoseEvaluationService evaluation_service;
	bool passed = true;

	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		const McsMv22KinematicState neutral_state =
			evaluation_service.createNeutralState(variant);
		passed &= require(
			neutral_state.wheelStates().size() == 4u,
			variant.identifier() + " neutral state must own four wheels");
		passed &= require(
			neutral_state.closureStates().size() == 6u,
			variant.identifier() + " neutral state must own six closures");
		passed &= require(
			neutral_state.glassStates().size() == 4u,
			variant.identifier() + " neutral state must own four glass carriers");

		const McsMv22EvaluatedKinematicState first_neutral =
			evaluation_service.evaluate(variant, family.sourceRelease(), neutral_state);
		const McsMv22EvaluatedKinematicState full_state =
			evaluation_service.evaluate(
				variant, family.sourceRelease(), createFullState(variant));
		const McsMv22EvaluatedKinematicState second_neutral =
			evaluation_service.evaluate(variant, family.sourceRelease(), neutral_state);

		passed &= require(
			first_neutral.wheelStates().size() == 4u &&
			first_neutral.closureProgen3dTransforms().size() == 6u &&
			first_neutral.glassProgen3dTransforms().size() == 4u,
			variant.identifier() + " evaluated state must preserve exact ownership");

		for (const auto &closure : first_neutral.closureProgen3dTransforms()) {
			passed &= require(
				isIdentity(closure.second),
				variant.identifier() + " neutral closure must be identity: " + closure.first);
			passed &= require(
				!isIdentity(full_state.closureProgen3dTransforms().at(closure.first), 1.0e-9),
				variant.identifier() + " full closure must move: " + closure.first);
			passed &= require(
				matricesMatch(
					closure.second,
					second_neutral.closureProgen3dTransforms().at(closure.first)),
				variant.identifier() + " closure evaluation must be immutable: " + closure.first);
		}

		for (const auto &glass : first_neutral.glassProgen3dTransforms()) {
			passed &= require(
				isIdentity(glass.second),
				variant.identifier() + " neutral glass must be identity: " + glass.first);
			const glm::dmat4 &full_transform =
				full_state.glassProgen3dTransforms().at(glass.first);
			passed &= require(
				!isIdentity(full_transform, 1.0e-9),
				variant.identifier() + " full glass carrier must move: " + glass.first);
			passed &= require(
				full_transform[3][1] < -1.0e-6,
				variant.identifier() + " full glass carrier must lower in ProGen3d Y: " + glass.first);
			passed &= require(
				matricesMatch(
					glass.second,
					second_neutral.glassProgen3dTransforms().at(glass.first)),
				variant.identifier() + " glass evaluation must be immutable: " + glass.first);
		}

		for (const auto &wheel : full_state.wheelStates()) {
			const VehicleWheelPoseEvaluation &evaluation = wheel.second.sourceEvaluation();
			passed &= require(
				evaluation.steeringDegrees() >= variant.minimumSteerDegrees() &&
				evaluation.steeringDegrees() <= variant.maximumSteerDegrees() &&
				evaluation.suspensionTravelMetres() >= variant.minimumTravelMetres() &&
				evaluation.suspensionTravelMetres() <= variant.maximumTravelMetres(),
				variant.identifier() + " full wheel state must remain within declared intervals");
			passed &= require(
				matricesMatch(
					first_neutral.wheelStates().at(wheel.first).progen3dTransform(),
					second_neutral.wheelStates().at(wheel.first).progen3dTransform()),
				variant.identifier() + " wheel evaluation must be immutable: " + wheel.first);
		}

		std::map<std::string, McsMv22WheelControlState> missing_wheels =
			neutral_state.wheelStates();
		missing_wheels.erase(missing_wheels.begin());
		passed &= require(
			rejectsInvalidState([&]() {
				evaluation_service.evaluate(
					variant, family.sourceRelease(),
					McsMv22KinematicState(
						missing_wheels, neutral_state.closureStates(),
						neutral_state.glassStates()));
			}),
			variant.identifier() + " must reject missing wheel state");

		std::map<std::string, double> unknown_closures = neutral_state.closureStates();
		unknown_closures.erase(unknown_closures.begin());
		unknown_closures.emplace("unknown_closure", 0.0);
		passed &= require(
			rejectsInvalidState([&]() {
				evaluation_service.evaluate(
					variant, family.sourceRelease(),
					McsMv22KinematicState(
						neutral_state.wheelStates(), unknown_closures,
						neutral_state.glassStates()));
			}),
			variant.identifier() + " must reject unknown closure state");

		std::map<std::string, double> invalid_glass = neutral_state.glassStates();
		invalid_glass.begin()->second = 1.01;
		passed &= require(
			rejectsInvalidState([&]() {
				evaluation_service.evaluate(
					variant, family.sourceRelease(),
					McsMv22KinematicState(
						neutral_state.wheelStates(), neutral_state.closureStates(),
						invalid_glass));
			}),
			variant.identifier() + " must reject out-of-range glass state");
	}

	if (!passed) return 1;
	std::cout << "MCSMv2.2 selected-state evaluation checks passed.\n";
	return 0;
}
