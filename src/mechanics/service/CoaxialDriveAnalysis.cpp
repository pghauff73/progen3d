#include "mechanics/service/CoaxialDriveAnalysis.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <cmath>
#include <vector>

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double frameTolerance = 1e-5;

bool rigidVerticalShaft(const glm::dmat4 &frame)
{
	const glm::dmat3 basis(frame);
	const auto gram = glm::transpose(basis) * basis;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			if (!(std::abs(gram[i][j] - (i == j ? 1.0 : 0.0)) <= frameTolerance)) return false;
	return glm::determinant(basis) > 0 &&
	       glm::length(basis[1] - glm::dvec3(0, 1, 0)) <= frameTolerance;
}

std::string inputError(const CoaxialDriveStage &s)
{
	for (double v : {s.hoopMassKg, s.effectiveRadiusM, s.relativeDegreesPerS, s.relativeDegreesPerS2,
	                 s.signedSpeedRatio, s.drivingEfficiency, s.backdriveEfficiency, s.rotorInertiaKgM2, s.runningDragNm})
		if (!std::isfinite(v)) return "All drive inputs must be finite.";
	if (s.hoopMassKg <= 0 || s.effectiveRadiusM <= 0) return "Hoop mass and effective radius must be positive.";
	if (s.signedSpeedRatio == 0) return "The signed shaft speed ratio must be nonzero.";
	if (s.drivingEfficiency <= 0 || s.drivingEfficiency > 1 || s.backdriveEfficiency <= 0 || s.backdriveEfficiency > 1)
		return "Both transmission efficiencies must be in (0, 1].";
	if (s.rotorInertiaKgM2 < 0 || s.runningDragNm < 0) return "Rotor inertia and running drag must be nonnegative.";
	return {};
}

// Unmarked objects between stages are rigid, massless adapters.
void nextStages(const SpatialBuildingModel &model, const SpatialObjectId &parent,
                std::vector<const SpatialBuildingObject *> &found)
{
	for (const auto &id : model.containmentTree().childrenOf(parent)) {
		const auto *child = model.objects().find(id);
		if (child->coaxialDrive()) found.push_back(child);
		else nextStages(model, id, found);
	}
}

double motionSign(double speed, double acceleration)
{
	const double direction = speed == 0 ? acceleration : speed;
	return (direction > 0) - (direction < 0);
}
}

CoaxialDriveAnalysis::CoaxialDriveAnalysis(const SpatialBuildingModel &model, const SpatialBuildingObject &selected)
{
	if (!selected.coaxialDrive()) return;
	const auto *first = &selected;
	const auto &tree = model.containmentTree();
	for (auto id = tree.parentOf(selected.identity().objectId()); id; id = tree.parentOf(*id)) {
		const auto *parent = model.objects().find(*id);
		if (parent->coaxialDrive()) first = parent;
	}
	const glm::dvec3 origin(first->frameState().resolvedWorldTransform()[3]);
	std::vector<const SpatialBuildingObject *> stages;
	std::vector<CoaxialDriveResponse> results;
	std::size_t selectedIndex = 0;
	double worldSpeed = 0, worldAcceleration = 0, hoopEnergy = 0, rotorEnergy = 0;
	for (auto stage = first; stage;) {
		const auto &id = stage->identity().objectId();
		const auto &s = *stage->coaxialDrive();
		std::string issue = inputError(s);
		const glm::dmat4 frame(stage->frameState().resolvedWorldTransform());
		if (!rigidVerticalShaft(frame)) issue = "Drive frames must be rigid with local +Y parallel to world +Y.";
		if (!(std::hypot(frame[3].x-origin.x, frame[3].z-origin.z) <= frameTolerance))
			issue = "Drive origins must lie on the same vertical shaft line.";
		if (!issue.empty()) { diagnostic_ = id.value() + ": " + issue; return; }
		if (id == selected.identity().objectId()) selectedIndex = stages.size();
		stages.push_back(stage);
		CoaxialDriveResponse r{};
		r.hoopInertiaKgM2 = 0.5 * s.hoopMassKg * s.effectiveRadiusM * s.effectiveRadiusM;
		r.relativeRadPerS = s.relativeDegreesPerS * pi / 180;
		const double relativeAcceleration = s.relativeDegreesPerS2 * pi / 180;
		const double motorSpeed = s.signedSpeedRatio * r.relativeRadPerS;
		r.motorRpm = motorSpeed * 30 / pi;
		r.rotorWorldRadPerS = worldSpeed + motorSpeed;
		r.rotorWorldRadPerS2 = worldAcceleration + s.signedSpeedRatio * relativeAcceleration;
		worldSpeed += r.relativeRadPerS;
		worldAcceleration += relativeAcceleration;
		r.worldRadPerS = worldSpeed;
		r.worldRadPerS2 = worldAcceleration;
		r.hoopEnergyJ = 0.5 * r.hoopInertiaKgM2 * worldSpeed * worldSpeed;
		hoopEnergy += r.hoopEnergyJ;
		rotorEnergy += 0.5 * s.rotorInertiaKgM2 * r.rotorWorldRadPerS * r.rotorWorldRadPerS;
		results.push_back(r);
		std::vector<const SpatialBuildingObject *> children;
		nextStages(model, id, children);
		if (children.size() > 1) { diagnostic_ = id.value() + ": Drive study requires a serial chain."; return; }
		stage = children.empty() ? nullptr : children.front();
	}

	double carriedHoopTorque = 0, carriedRotorTorque = 0;
	for (std::size_t i = stages.size(); i-- > 0;) {
		const auto &s = *stages[i]->coaxialDrive();
		auto &r = results[i];
		carriedHoopTorque += r.hoopInertiaKgM2 * r.worldRadPerS2;
		r.hoopJointTorqueNm = carriedHoopTorque;
		const double direction = motionSign(s.relativeDegreesPerS, s.relativeDegreesPerS2);
		r.gearboxOutputTorqueNm = carriedHoopTorque + carriedRotorTorque + direction * s.runningDragNm;
		const double powerDirection = r.gearboxOutputTorqueNm * direction;
		double gearboxInputTorque = r.gearboxOutputTorqueNm / s.signedSpeedRatio;
		if (powerDirection > 0) gearboxInputTorque /= s.drivingEfficiency;
		if (powerDirection < 0) gearboxInputTorque *= s.backdriveEfficiency;
		const double rotorTorque = s.rotorInertiaKgM2 * r.rotorWorldRadPerS2;
		r.motorTorqueNm = gearboxInputTorque + rotorTorque;
		r.motorPowerW = r.motorTorqueNm * s.signedSpeedRatio * r.relativeRadPerS;
		r.chainHoopEnergyJ = hoopEnergy;
		r.chainRotorEnergyJ = rotorEnergy;
		carriedRotorTorque += rotorTorque;
	}
	response_ = results[selectedIndex];
}
