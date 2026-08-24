#pragma once

class VehicleWheelPoseSolutionPolicy
{
public:
	VehicleWheelPoseSolutionPolicy(
		bool steering_enabled,
		double static_camber_degrees,
		double camber_gain_degrees_per_metre,
		double toe_gain_degrees_per_metre,
		double longitudinal_centre_gain,
		double lateral_track_gain)
		: steering_enabled_(steering_enabled),
		  static_camber_degrees_(static_camber_degrees),
		  camber_gain_degrees_per_metre_(camber_gain_degrees_per_metre),
		  toe_gain_degrees_per_metre_(toe_gain_degrees_per_metre),
		  longitudinal_centre_gain_(longitudinal_centre_gain),
		  lateral_track_gain_(lateral_track_gain)
	{
	}

	bool steeringEnabled() const { return steering_enabled_; }
	double staticCamberDegrees() const { return static_camber_degrees_; }
	double camberGainDegreesPerMetre() const
	{
		return camber_gain_degrees_per_metre_;
	}
	double toeGainDegreesPerMetre() const
	{
		return toe_gain_degrees_per_metre_;
	}
	double longitudinalCentreGain() const
	{
		return longitudinal_centre_gain_;
	}
	double lateralTrackGain() const { return lateral_track_gain_; }

private:
	bool steering_enabled_ = false;
	double static_camber_degrees_ = 0.0;
	double camber_gain_degrees_per_metre_ = 0.0;
	double toe_gain_degrees_per_metre_ = 0.0;
	double longitudinal_centre_gain_ = 0.0;
	double lateral_track_gain_ = 0.0;
};
