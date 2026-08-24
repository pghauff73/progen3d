#include "vehicle/mcsmv2/model/VehicleSectionLandmarkRole.h"
#include "vehicle/mcsmv2/model/VehicleCharacterCurve.h"
#include "vehicle/mcsmv2/model/VehicleSemanticStationRole.h"

const char *vehicleSemanticStationRoleName(VehicleSemanticStationRole role)
{
	switch (role) {
	case VehicleSemanticStationRole::TailFace: return "tail_face";
	case VehicleSemanticStationRole::RearBumper: return "rear_bumper";
	case VehicleSemanticStationRole::RearAxle: return "rear_axle";
	case VehicleSemanticStationRole::RearDoor: return "rear_door";
	case VehicleSemanticStationRole::BPillar: return "b_pillar";
	case VehicleSemanticStationRole::FrontDoor: return "front_door";
	case VehicleSemanticStationRole::APillar: return "a_pillar";
	case VehicleSemanticStationRole::HoodRear: return "hood_rear";
	case VehicleSemanticStationRole::FrontAxle: return "front_axle";
	case VehicleSemanticStationRole::Nose: return "nose";
	case VehicleSemanticStationRole::FrontFace: return "front_face";
	}
	return "unknown";
}

const char *vehicleSectionLandmarkRoleName(VehicleSectionLandmarkRole role)
{
	switch (role) {
	case VehicleSectionLandmarkRole::Underbody: return "underbody";
	case VehicleSectionLandmarkRole::Rocker: return "rocker";
	case VehicleSectionLandmarkRole::LowerBody: return "lower_body";
	case VehicleSectionLandmarkRole::Shoulder: return "shoulder";
	case VehicleSectionLandmarkRole::Belt: return "belt";
	case VehicleSectionLandmarkRole::GlassShoulder: return "glass_shoulder";
	case VehicleSectionLandmarkRole::RoofRail: return "roof_rail";
	case VehicleSectionLandmarkRole::RoofCrown: return "roof_crown";
	}
	return "unknown";
}

const char *vehicleCharacterCurveRoleName(VehicleCharacterCurveRole role)
{
	switch (role) {
	case VehicleCharacterCurveRole::CentreSpine: return "centre_spine";
	case VehicleCharacterCurveRole::RoofCentre: return "roof_centre";
	case VehicleCharacterCurveRole::RoofRailLeft: return "roof_rail_left";
	case VehicleCharacterCurveRole::RoofRailRight: return "roof_rail_right";
	case VehicleCharacterCurveRole::GlassShoulderLeft: return "glass_shoulder_left";
	case VehicleCharacterCurveRole::GlassShoulderRight: return "glass_shoulder_right";
	case VehicleCharacterCurveRole::BeltLeft: return "belt_left";
	case VehicleCharacterCurveRole::BeltRight: return "belt_right";
	case VehicleCharacterCurveRole::ShoulderLeft: return "shoulder_left";
	case VehicleCharacterCurveRole::ShoulderRight: return "shoulder_right";
	case VehicleCharacterCurveRole::RockerLeft: return "rocker_left";
	case VehicleCharacterCurveRole::RockerRight: return "rocker_right";
	case VehicleCharacterCurveRole::UnderbodyEdgeLeft: return "underbody_edge_left";
	case VehicleCharacterCurveRole::UnderbodyEdgeRight: return "underbody_edge_right";
	}
	return "unknown";
}
