#pragma once

class ModernCarClosureDefinition
{
public:
	ModernCarClosureDefinition(
		double front_door_maximum_degrees,
		double rear_door_maximum_degrees,
		double bonnet_maximum_degrees,
		double hatch_maximum_degrees,
		double side_glass_travel,
		double nominal_panel_gap)
		: front_door_maximum_degrees_(front_door_maximum_degrees),
		  rear_door_maximum_degrees_(rear_door_maximum_degrees),
		  bonnet_maximum_degrees_(bonnet_maximum_degrees),
		  hatch_maximum_degrees_(hatch_maximum_degrees),
		  side_glass_travel_(side_glass_travel),
		  nominal_panel_gap_(nominal_panel_gap)
	{
	}

	double frontDoorMaximumDegrees() const { return front_door_maximum_degrees_; }
	double rearDoorMaximumDegrees() const { return rear_door_maximum_degrees_; }
	double bonnetMaximumDegrees() const { return bonnet_maximum_degrees_; }
	double hatchMaximumDegrees() const { return hatch_maximum_degrees_; }
	double sideGlassTravel() const { return side_glass_travel_; }
	double nominalPanelGap() const { return nominal_panel_gap_; }

private:
	double front_door_maximum_degrees_ = 0.0;
	double rear_door_maximum_degrees_ = 0.0;
	double bonnet_maximum_degrees_ = 0.0;
	double hatch_maximum_degrees_ = 0.0;
	double side_glass_travel_ = 0.0;
	double nominal_panel_gap_ = 0.0;
};
