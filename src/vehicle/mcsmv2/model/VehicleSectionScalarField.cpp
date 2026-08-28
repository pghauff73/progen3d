#include "vehicle/mcsmv2/model/VehicleSectionScalarField.h"

const char *vehicleSectionScalarFieldName(VehicleSectionScalarField field)
{
	switch (field) {
	case VehicleSectionScalarField::UnderbodyHeight: return "underbody_z";
	case VehicleSectionScalarField::UnderbodyHalfWidth: return "underbody_halfwidth";
	case VehicleSectionScalarField::RockerHeight: return "rocker_z";
	case VehicleSectionScalarField::RockerHalfWidth: return "rocker_halfwidth";
	case VehicleSectionScalarField::LowerBodyHeight: return "lower_z";
	case VehicleSectionScalarField::LowerBodyHalfWidth: return "lower_halfwidth";
	case VehicleSectionScalarField::ShoulderHeight: return "shoulder_z";
	case VehicleSectionScalarField::ShoulderHalfWidth: return "shoulder_halfwidth";
	case VehicleSectionScalarField::BeltHeight: return "belt_z";
	case VehicleSectionScalarField::BeltHalfWidth: return "belt_halfwidth";
	case VehicleSectionScalarField::GlassShoulderHeight: return "glass_shoulder_z";
	case VehicleSectionScalarField::GlassShoulderHalfWidth: return "glass_shoulder_halfwidth";
	case VehicleSectionScalarField::RoofRailHeight: return "roof_rail_z";
	case VehicleSectionScalarField::RoofRailHalfWidth: return "roof_rail_halfwidth";
	case VehicleSectionScalarField::RoofCrownHeight: return "roof_crown_z";
	}
	return "unknown";
}
