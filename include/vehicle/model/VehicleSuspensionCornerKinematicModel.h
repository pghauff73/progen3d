#pragma once

#include "vehicle/model/SuspensionCornerSpecification.h"
#include "vehicle/model/VehicleWheelPoseSolutionPolicy.h"

#include <glm/glm.hpp>

#include <utility>

class VehicleSuspensionCornerKinematicModel
{
public:
	VehicleSuspensionCornerKinematicModel(
		VehicleCornerLocation corner,
		glm::dvec3 nominal_source_hub_centre,
		double source_track_width_metres,
		double wheel_radius_metres,
		double minimum_steering_degrees,
		double maximum_steering_degrees,
		double minimum_travel_metres,
		double maximum_travel_metres,
		VehicleWheelPoseSolutionPolicy solution_policy)
		: corner_(corner),
		  nominal_source_hub_centre_(nominal_source_hub_centre),
		  source_track_width_metres_(source_track_width_metres),
		  wheel_radius_metres_(wheel_radius_metres),
		  minimum_steering_degrees_(minimum_steering_degrees),
		  maximum_steering_degrees_(maximum_steering_degrees),
		  minimum_travel_metres_(minimum_travel_metres),
		  maximum_travel_metres_(maximum_travel_metres),
		  solution_policy_(std::move(solution_policy))
	{
	}

	VehicleCornerLocation corner() const { return corner_; }
	const glm::dvec3 &nominalSourceHubCentre() const
	{
		return nominal_source_hub_centre_;
	}
	double sourceTrackWidthMetres() const { return source_track_width_metres_; }
	double wheelRadiusMetres() const { return wheel_radius_metres_; }
	double minimumSteeringDegrees() const { return minimum_steering_degrees_; }
	double maximumSteeringDegrees() const { return maximum_steering_degrees_; }
	double minimumTravelMetres() const { return minimum_travel_metres_; }
	double maximumTravelMetres() const { return maximum_travel_metres_; }
	const VehicleWheelPoseSolutionPolicy &solutionPolicy() const
	{
		return solution_policy_;
	}

private:
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	glm::dvec3 nominal_source_hub_centre_{0.0};
	double source_track_width_metres_ = 0.0;
	double wheel_radius_metres_ = 0.0;
	double minimum_steering_degrees_ = 0.0;
	double maximum_steering_degrees_ = 0.0;
	double minimum_travel_metres_ = 0.0;
	double maximum_travel_metres_ = 0.0;
	VehicleWheelPoseSolutionPolicy solution_policy_{false, 0.0, 0.0, 0.0, 0.0, 0.0};
};
