#pragma once

class ModernCarPlatformDefinition
{
public:
	ModernCarPlatformDefinition(
		double floor_height,
		double front_hpoint_x,
		double rear_hpoint_x,
		double hpoint_height,
		double eye_height,
		double head_clearance,
		double seat_lateral_offset)
		: floor_height_(floor_height),
		  front_hpoint_x_(front_hpoint_x),
		  rear_hpoint_x_(rear_hpoint_x),
		  hpoint_height_(hpoint_height),
		  eye_height_(eye_height),
		  head_clearance_(head_clearance),
		  seat_lateral_offset_(seat_lateral_offset)
	{
	}

	double floorHeight() const { return floor_height_; }
	double frontHpointX() const { return front_hpoint_x_; }
	double rearHpointX() const { return rear_hpoint_x_; }
	double hpointHeight() const { return hpoint_height_; }
	double eyeHeight() const { return eye_height_; }
	double headClearance() const { return head_clearance_; }
	double seatLateralOffset() const { return seat_lateral_offset_; }

private:
	double floor_height_ = 0.0;
	double front_hpoint_x_ = 0.0;
	double rear_hpoint_x_ = 0.0;
	double hpoint_height_ = 0.0;
	double eye_height_ = 0.0;
	double head_clearance_ = 0.0;
	double seat_lateral_offset_ = 0.0;
};
