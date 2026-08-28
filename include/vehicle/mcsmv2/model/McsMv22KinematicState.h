#pragma once

#include <map>
#include <string>
#include <utility>

class McsMv22WheelControlState
{
public:
	McsMv22WheelControlState(
		double steering_degrees,
		double suspension_travel_metres)
		: steering_degrees_(steering_degrees),
		  suspension_travel_metres_(suspension_travel_metres)
	{
	}

	double steeringDegrees() const { return steering_degrees_; }
	double suspensionTravelMetres() const
	{
		return suspension_travel_metres_;
	}

private:
	double steering_degrees_ = 0.0;
	double suspension_travel_metres_ = 0.0;
};

class McsMv22KinematicState
{
public:
	McsMv22KinematicState(
		std::map<std::string, McsMv22WheelControlState> wheel_states,
		std::map<std::string, double> closure_states,
		std::map<std::string, double> glass_states)
		: wheel_states_(std::move(wheel_states)),
		  closure_states_(std::move(closure_states)),
		  glass_states_(std::move(glass_states))
	{
	}

	const std::map<std::string, McsMv22WheelControlState> &wheelStates() const
	{
		return wheel_states_;
	}
	const std::map<std::string, double> &closureStates() const
	{
		return closure_states_;
	}
	const std::map<std::string, double> &glassStates() const
	{
		return glass_states_;
	}

private:
	std::map<std::string, McsMv22WheelControlState> wheel_states_;
	std::map<std::string, double> closure_states_;
	std::map<std::string, double> glass_states_;
};
