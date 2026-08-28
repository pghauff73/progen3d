#pragma once

class VehicleWheelParameters
{
public:
	VehicleWheelParameters(
		double radius,
		double width,
		double front_track,
		double rear_track)
		: radius_(radius),
		  width_(width),
		  front_track_(front_track),
		  rear_track_(rear_track)
	{
	}

	double radius() const { return radius_; }
	double width() const { return width_; }
	double frontTrack() const { return front_track_; }
	double rearTrack() const { return rear_track_; }

private:
	double radius_ = 0.0;
	double width_ = 0.0;
	double front_track_ = 0.0;
	double rear_track_ = 0.0;
};
