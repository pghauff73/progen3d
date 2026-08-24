#pragma once

#include "vehicle/model/VehicleWheelPoseEvaluation.h"

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <utility>

class McsMv22EvaluatedWheelState
{
public:
	McsMv22EvaluatedWheelState(
		VehicleWheelPoseEvaluation source_evaluation,
		glm::dmat4 progen3d_transform)
		: source_evaluation_(std::move(source_evaluation)),
		  progen3d_transform_(progen3d_transform)
	{
	}

	const VehicleWheelPoseEvaluation &sourceEvaluation() const
	{
		return source_evaluation_;
	}
	const glm::dmat4 &progen3dTransform() const { return progen3d_transform_; }

private:
	VehicleWheelPoseEvaluation source_evaluation_{
		VehicleCornerLocation::FrontLeft, 0.0, 0.0, 0.0, 0.0, {}, {}};
	glm::dmat4 progen3d_transform_{1.0};
};

class McsMv22EvaluatedKinematicState
{
public:
	McsMv22EvaluatedKinematicState(
		std::map<std::string, McsMv22EvaluatedWheelState> wheel_states,
		std::map<std::string, glm::dmat4> closure_progen3d_transforms,
		std::map<std::string, glm::dmat4> glass_progen3d_transforms)
		: wheel_states_(std::move(wheel_states)),
		  closure_progen3d_transforms_(std::move(closure_progen3d_transforms)),
		  glass_progen3d_transforms_(std::move(glass_progen3d_transforms))
	{
	}

	const std::map<std::string, McsMv22EvaluatedWheelState> &wheelStates() const
	{
		return wheel_states_;
	}
	const std::map<std::string, glm::dmat4> &closureProgen3dTransforms() const
	{
		return closure_progen3d_transforms_;
	}
	const std::map<std::string, glm::dmat4> &glassProgen3dTransforms() const
	{
		return glass_progen3d_transforms_;
	}

private:
	std::map<std::string, McsMv22EvaluatedWheelState> wheel_states_;
	std::map<std::string, glm::dmat4> closure_progen3d_transforms_;
	std::map<std::string, glm::dmat4> glass_progen3d_transforms_;
};
