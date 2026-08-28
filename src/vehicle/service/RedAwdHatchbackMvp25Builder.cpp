#include "vehicle/service/RedAwdHatchbackMvp25Builder.h"

#include "vehicle/service/AutomotiveClosureGeometryService.h"
#include "vehicle/service/RedAwdHatchbackMvpv2Builder.h"
#include "vehicle/service/VehicleChassisKinematicsService.h"
#include "vehicle/service/VehicleClassAValidationService.h"
#include "vehicle/service/VehicleFitObjectiveService.h"
#include "vehicle/service/VehicleMvp25DeterministicHashService.h"
#include "vehicle/service/VehicleMvp25ValidationService.h"
#include "vehicle/service/VehicleProjectionService.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

AxisAlignedBounds create_bounds(glm::vec3 minimum, glm::vec3 maximum)
{
	AxisAlignedBounds bounds;
	bounds.min = minimum;
	bounds.max = maximum;
	bounds.center = (minimum + maximum) * 0.5f;
	bounds.half_extents = (maximum - minimum) * 0.5f;
	bounds.valid = true;
	return bounds;
}

glm::mat4 create_front_camera_transform()
{
	return glm::mat4(1.0f);
}

glm::mat4 create_rear_camera_transform()
{
	glm::mat4 transform(1.0f);
	transform[0][0] = -1.0f;
	transform[2][2] = -1.0f;
	return transform;
}

glm::mat4 create_left_camera_transform()
{
	glm::mat4 transform(0.0f);
	transform[2][0] = 1.0f;
	transform[1][1] = 1.0f;
	transform[0][2] = 1.0f;
	transform[3][3] = 1.0f;
	return transform;
}

glm::mat4 create_right_camera_transform()
{
	glm::mat4 transform(0.0f);
	transform[2][0] = -1.0f;
	transform[1][1] = 1.0f;
	transform[0][2] = -1.0f;
	transform[3][3] = 1.0f;
	return transform;
}

glm::mat4 create_top_camera_transform()
{
	glm::mat4 transform(0.0f);
	transform[0][0] = 1.0f;
	transform[2][1] = -1.0f;
	transform[1][2] = 1.0f;
	transform[3][3] = 1.0f;
	return transform;
}

std::vector<VehicleCameraModel> create_cameras()
{
	return {
		{"Camera.Front", VehicleReferenceView::Front,
		 VehicleCameraProjection::Orthographic, create_front_camera_transform(),
		 {0.5f, 0.18f}, 1.0f, 5.0f, {1.0f, 1.0f}},
		{"Camera.Rear", VehicleReferenceView::Rear,
		 VehicleCameraProjection::Orthographic, create_rear_camera_transform(),
		 {0.5f, 0.18f}, 1.0f, 5.0f, {1.0f, 1.0f}},
		{"Camera.Left", VehicleReferenceView::Left,
		 VehicleCameraProjection::Orthographic, create_left_camera_transform(),
		 {0.5f, 0.18f}, 1.0f, 5.0f, {1.0f, 1.0f}},
		{"Camera.Right", VehicleReferenceView::Right,
		 VehicleCameraProjection::Orthographic, create_right_camera_transform(),
		 {0.5f, 0.18f}, 1.0f, 5.0f, {1.0f, 1.0f}},
		{"Camera.Top", VehicleReferenceView::Top,
		 VehicleCameraProjection::Orthographic, create_top_camera_transform(),
		 {0.5f, 0.5f}, 1.0f, 5.0f, {1.0f, 1.0f}}};
}

std::string reference_image_identifier_for(const VehicleCameraModel &camera)
{
	const std::string reference_path =
		"examples/MVPv2_5_Red_AWD_Hatchback/MVP2.5sportyredAWD.png";
	switch (camera.view()) {
	case VehicleReferenceView::Front:
		return reference_path + "#front(0,0,492,509)";
	case VehicleReferenceView::Rear:
		return reference_path + "#back(492,0,428,509)";
	case VehicleReferenceView::Right:
		return reference_path + "#right(920,0,616,509)";
	case VehicleReferenceView::Left:
		return reference_path + "#left(0,509,492,515)";
	case VehicleReferenceView::Top:
		return reference_path + "#top(492,509,584,515)";
	}
	return reference_path;
}

struct LandmarkDefinition {
	const char *identifier;
	glm::vec3 position;
	const char *symmetry_partner;
	const char *topology_role;
};

std::vector<LandmarkDefinition> create_landmark_definitions()
{
	return {
		{"FrontWheelCentreL", {-0.75f, 0.32f, 1.325f}, "FrontWheelCentreR", "WheelCentre"},
		{"FrontWheelCentreR", {0.75f, 0.32f, 1.325f}, "FrontWheelCentreL", "WheelCentre"},
		{"RearWheelCentreL", {-0.75f, 0.32f, -1.325f}, "RearWheelCentreR", "WheelCentre"},
		{"RearWheelCentreR", {0.75f, 0.32f, -1.325f}, "RearWheelCentreL", "WheelCentre"},
		{"HoodFrontCentre", {0.0f, 0.82f, 2.05f}, "", "CentreSpine"},
		{"HoodRearCentre", {0.0f, 0.94f, 0.82f}, "", "CentreSpine"},
		{"HeadlampInnerL", {-0.28f, 0.69f, 2.12f}, "HeadlampInnerR", "Lamp"},
		{"HeadlampInnerR", {0.28f, 0.69f, 2.12f}, "HeadlampInnerL", "Lamp"},
		{"HeadlampOuterL", {-0.77f, 0.66f, 2.00f}, "HeadlampOuterR", "Lamp"},
		{"HeadlampOuterR", {0.77f, 0.66f, 2.00f}, "HeadlampOuterL", "Lamp"},
		{"APillarBaseL", {-0.78f, 0.88f, 0.65f}, "APillarBaseR", "Pillar"},
		{"APillarBaseR", {0.78f, 0.88f, 0.65f}, "APillarBaseL", "Pillar"},
		{"APillarRoofL", {-0.62f, 1.34f, 0.35f}, "APillarRoofR", "Pillar"},
		{"APillarRoofR", {0.62f, 1.34f, 0.35f}, "APillarRoofL", "Pillar"},
		{"BPillarBeltL", {-0.84f, 0.88f, -0.25f}, "BPillarBeltR", "Pillar"},
		{"BPillarBeltR", {0.84f, 0.88f, -0.25f}, "BPillarBeltL", "Pillar"},
		{"BPillarRoofL", {-0.64f, 1.41f, -0.30f}, "BPillarRoofR", "Pillar"},
		{"BPillarRoofR", {0.64f, 1.41f, -0.30f}, "BPillarRoofL", "Pillar"},
		{"CPillarBeltL", {-0.82f, 0.87f, -1.26f}, "CPillarBeltR", "Pillar"},
		{"CPillarBeltR", {0.82f, 0.87f, -1.26f}, "CPillarBeltL", "Pillar"},
		{"CPillarRoofL", {-0.58f, 1.30f, -1.30f}, "CPillarRoofR", "Pillar"},
		{"CPillarRoofR", {0.58f, 1.30f, -1.30f}, "CPillarRoofL", "Pillar"},
		{"RoofFrontCentre", {0.0f, 1.36f, 0.35f}, "", "CentreSpine"},
		{"RoofApex", {0.0f, 1.43f, -0.45f}, "", "CentreSpine"},
		{"RoofRearCentre", {0.0f, 1.27f, -1.48f}, "", "CentreSpine"},
		{"TailLampInnerL", {-0.25f, 0.75f, -2.08f}, "TailLampInnerR", "Lamp"},
		{"TailLampInnerR", {0.25f, 0.75f, -2.08f}, "TailLampInnerL", "Lamp"},
		{"TailLampOuterL", {-0.78f, 0.74f, -1.97f}, "TailLampOuterR", "Lamp"},
		{"TailLampOuterR", {0.78f, 0.74f, -1.97f}, "TailLampOuterL", "Lamp"},
		{"SpoilerTipL", {-0.63f, 1.32f, -1.72f}, "SpoilerTipR", "Spoiler"},
		{"SpoilerTipR", {0.63f, 1.32f, -1.72f}, "SpoilerTipL", "Spoiler"}};
}

std::vector<AutomotiveCharacterCurve> create_semantic_character_curves()
{
	struct CurveDefinition {
		const char *identifier;
		const char *role;
		std::vector<glm::vec3> points;
	};
	const std::vector<CurveDefinition> definitions = {
		{"RoofCrown", "RoofCrown", {{0.0f, 1.36f, 0.35f}, {0.0f, 1.43f, -0.45f}, {0.0f, 1.27f, -1.48f}}},
		{"BeltLineL", "BeltLine", {{-0.80f, 0.86f, 0.70f}, {-0.84f, 0.88f, -0.25f}, {-0.82f, 0.87f, -1.30f}}},
		{"BeltLineR", "BeltLine", {{0.80f, 0.86f, 0.70f}, {0.84f, 0.88f, -0.25f}, {0.82f, 0.87f, -1.30f}}},
		{"UpperShoulderL", "UpperShoulder", {{-0.86f, 0.78f, 1.65f}, {-0.89f, 0.82f, 0.10f}, {-0.86f, 0.80f, -1.65f}}},
		{"UpperShoulderR", "UpperShoulder", {{0.86f, 0.78f, 1.65f}, {0.89f, 0.82f, 0.10f}, {0.86f, 0.80f, -1.65f}}},
		{"LowerShoulderL", "LowerShoulder", {{-0.88f, 0.55f, 1.50f}, {-0.90f, 0.52f, 0.0f}, {-0.86f, 0.54f, -1.60f}}},
		{"LowerShoulderR", "LowerShoulder", {{0.88f, 0.55f, 1.50f}, {0.90f, 0.52f, 0.0f}, {0.86f, 0.54f, -1.60f}}},
		{"RockerL", "Rocker", {{-0.82f, 0.20f, 1.05f}, {-0.86f, 0.19f, 0.0f}, {-0.82f, 0.21f, -1.25f}}},
		{"RockerR", "Rocker", {{0.82f, 0.20f, 1.05f}, {0.86f, 0.19f, 0.0f}, {0.82f, 0.21f, -1.25f}}},
		{"HoodCentre", "HoodCentre", {{0.0f, 0.82f, 2.08f}, {0.0f, 0.88f, 1.45f}, {0.0f, 0.94f, 0.82f}}},
		{"HoodOuterL", "HoodOuter", {{-0.68f, 0.78f, 2.02f}, {-0.72f, 0.84f, 1.38f}, {-0.66f, 0.91f, 0.82f}}},
		{"HoodOuterR", "HoodOuter", {{0.68f, 0.78f, 2.02f}, {0.72f, 0.84f, 1.38f}, {0.66f, 0.91f, 0.82f}}},
		{"FrontFenderCrownL", "FrontFenderCrown", {{-0.84f, 0.48f, 1.76f}, {-0.87f, 0.70f, 1.32f}, {-0.84f, 0.48f, 0.88f}}},
		{"FrontFenderCrownR", "FrontFenderCrown", {{0.84f, 0.48f, 1.76f}, {0.87f, 0.70f, 1.32f}, {0.84f, 0.48f, 0.88f}}},
		{"RearHaunchL", "RearHaunch", {{-0.84f, 0.62f, -0.82f}, {-0.90f, 0.78f, -1.32f}, {-0.83f, 0.67f, -1.82f}}},
		{"RearHaunchR", "RearHaunch", {{0.84f, 0.62f, -0.82f}, {0.90f, 0.78f, -1.32f}, {0.83f, 0.67f, -1.82f}}},
		{"WindowUpperL", "WindowUpper", {{-0.62f, 1.34f, 0.35f}, {-0.64f, 1.41f, -0.30f}, {-0.58f, 1.30f, -1.30f}}},
		{"WindowUpperR", "WindowUpper", {{0.62f, 1.34f, 0.35f}, {0.64f, 1.41f, -0.30f}, {0.58f, 1.30f, -1.30f}}},
		{"WindowLowerL", "WindowLower", {{-0.78f, 0.88f, 0.65f}, {-0.84f, 0.88f, -0.25f}, {-0.82f, 0.87f, -1.26f}}},
		{"WindowLowerR", "WindowLower", {{0.78f, 0.88f, 0.65f}, {0.84f, 0.88f, -0.25f}, {0.82f, 0.87f, -1.26f}}}};
	std::vector<AutomotiveCharacterCurve> curves;
	for (const CurveDefinition &definition : definitions) {
		std::vector<std::string> observation_identifiers;
		for (const char *camera : {"Camera.Front", "Camera.Rear", "Camera.Left", "Camera.Right", "Camera.Top"}) {
			observation_identifiers.push_back(
				std::string(definition.identifier) + ".Observation." + camera);
		}
		curves.emplace_back(
			definition.identifier, definition.role, 2, definition.points,
			std::move(observation_identifiers), std::vector<std::string>{});
	}
	return curves;
}

std::vector<glm::vec2> project_points(
	const VehicleCameraModel &camera,
	const std::vector<glm::vec3> &points)
{
	std::vector<glm::vec2> projected;
	for (const glm::vec3 &point : points) {
		const auto image_point = VehicleProjectionService().projectPoint(camera, point);
		if (image_point.has_value()) projected.push_back(*image_point);
	}
	return projected;
}

CharacterLineObservation create_projected_line_observation(
	const std::string &identifier,
	const VehicleCameraModel &camera,
	const std::vector<glm::vec3> &model_points)
{
	return CharacterLineObservation(
		identifier + ".Observation." + camera.identifier(), camera.identifier(),
		project_points(camera, model_points), 0.002f, 1.0f);
}

std::vector<CharacterLineObservation> create_panel_line_observations(
	const VehicleCameraModel &camera)
{
	const std::vector<std::pair<std::string, std::vector<glm::vec3>>> panel_lines = {
		{"PanelLine.FrontDoorL", {{-0.88f, 0.22f, -0.20f}, {-0.85f, 0.88f, -0.20f}}},
		{"PanelLine.FrontDoorR", {{0.88f, 0.22f, -0.20f}, {0.85f, 0.88f, -0.20f}}},
		{"PanelLine.RearDoorL", {{-0.87f, 0.22f, -1.25f}, {-0.83f, 0.87f, -1.25f}}},
		{"PanelLine.RearDoorR", {{0.87f, 0.22f, -1.25f}, {0.83f, 0.87f, -1.25f}}},
		{"PanelLine.HoodRear", {{-0.66f, 0.91f, 0.82f}, {0.0f, 0.94f, 0.82f}, {0.66f, 0.91f, 0.82f}}},
		{"PanelLine.RearHatch", {{-0.58f, 1.27f, -1.48f}, {0.0f, 1.25f, -1.52f}, {0.58f, 1.27f, -1.48f}}}};
	std::vector<CharacterLineObservation> observations;
	for (const auto &panel_line : panel_lines) {
		observations.push_back(create_projected_line_observation(
			panel_line.first, camera, panel_line.second));
	}
	return observations;
}

std::vector<CharacterLineObservation> create_glazing_line_observations(
	const VehicleCameraModel &camera)
{
	const std::vector<std::pair<std::string, std::vector<glm::vec3>>> glazing_lines = {
		{"GlazingLine.UpperL", {{-0.62f, 1.34f, 0.35f}, {-0.64f, 1.41f, -0.30f}, {-0.58f, 1.30f, -1.30f}}},
		{"GlazingLine.UpperR", {{0.62f, 1.34f, 0.35f}, {0.64f, 1.41f, -0.30f}, {0.58f, 1.30f, -1.30f}}},
		{"GlazingLine.LowerL", {{-0.78f, 0.88f, 0.65f}, {-0.84f, 0.88f, -0.25f}, {-0.82f, 0.87f, -1.26f}}},
		{"GlazingLine.LowerR", {{0.78f, 0.88f, 0.65f}, {0.84f, 0.88f, -0.25f}, {0.82f, 0.87f, -1.26f}}},
		{"GlazingLine.WindshieldHeader", {{-0.62f, 1.34f, 0.35f}, {0.0f, 1.36f, 0.35f}, {0.62f, 1.34f, 0.35f}}},
		{"GlazingLine.BacklightHeader", {{-0.58f, 1.30f, -1.30f}, {0.0f, 1.31f, -1.30f}, {0.58f, 1.30f, -1.30f}}}};
	std::vector<CharacterLineObservation> observations;
	for (const auto &glazing_line : glazing_lines) {
		observations.push_back(create_projected_line_observation(
			glazing_line.first, camera, glazing_line.second));
	}
	return observations;
}

std::vector<glm::vec3> create_wheel_ellipse_points(glm::vec3 wheel_centre)
{
	constexpr float pi = 3.14159265358979323846f;
	std::vector<glm::vec3> points;
	for (std::size_t sample = 0u; sample <= 16u; ++sample) {
		const float angle = 2.0f * pi * static_cast<float>(sample) / 16.0f;
		points.emplace_back(
			wheel_centre.x,
			wheel_centre.y + 0.32f * std::sin(angle),
			wheel_centre.z + 0.32f * std::cos(angle));
	}
	return points;
}

std::vector<CharacterLineObservation> create_wheel_ellipse_observations(
	const VehicleCameraModel &camera)
{
	const std::vector<std::pair<std::string, glm::vec3>> wheels = {
		{"WheelEllipse.FrontLeft", {-0.75f, 0.32f, 1.325f}},
		{"WheelEllipse.FrontRight", {0.75f, 0.32f, 1.325f}},
		{"WheelEllipse.RearLeft", {-0.75f, 0.32f, -1.325f}},
		{"WheelEllipse.RearRight", {0.75f, 0.32f, -1.325f}}};
	std::vector<CharacterLineObservation> observations;
	for (const auto &wheel : wheels) {
		observations.push_back(create_projected_line_observation(
			wheel.first, camera, create_wheel_ellipse_points(wheel.second)));
	}
	return observations;
}

VehicleObservationSet create_observation_set(
	const std::vector<VehicleCameraModel> &cameras,
	const std::vector<LandmarkDefinition> &landmarks,
	const std::vector<AutomotiveCharacterCurve> &semantic_curves)
{
	const std::vector<glm::vec3> silhouette_points = {
		{-0.90f, 0.12f, 2.175f}, {0.90f, 0.12f, 2.175f},
		{0.90f, 0.90f, 1.25f}, {0.65f, 1.43f, -0.40f},
		{0.58f, 1.27f, -1.65f}, {0.86f, 0.62f, -2.175f},
		{-0.86f, 0.62f, -2.175f}, {-0.58f, 1.27f, -1.65f},
		{-0.65f, 1.43f, -0.40f}, {-0.90f, 0.90f, 1.25f},
		{-0.90f, 0.12f, 2.175f}};
	std::vector<std::string> landmark_identifiers;
	for (const LandmarkDefinition &landmark : landmarks) {
		landmark_identifiers.emplace_back(landmark.identifier);
	}
	std::vector<VehicleViewObservation> observations;
	for (const VehicleCameraModel &camera : cameras) {
		std::vector<CharacterLineObservation> character_lines;
		for (const AutomotiveCharacterCurve &curve : semantic_curves) {
			character_lines.emplace_back(
				curve.identifier() + ".Observation." + camera.identifier(),
				camera.identifier(), project_points(camera, curve.controlPoints()),
				0.002f, 1.0f);
		}
		const std::vector<glm::vec2> silhouette = project_points(camera, silhouette_points);
		observations.emplace_back(
			"Observation." + camera.identifier(), camera,
			reference_image_identifier_for(camera), silhouette,
			landmark_identifiers, std::move(character_lines),
			create_panel_line_observations(camera),
			create_glazing_line_observations(camera),
			create_wheel_ellipse_observations(camera));
	}
	return VehicleObservationSet(std::move(observations));
}

std::vector<VehicleLandmark> create_landmarks(
	const std::vector<LandmarkDefinition> &definitions,
	const VehicleObservationSet &observations)
{
	std::vector<VehicleLandmark> landmarks;
	for (const LandmarkDefinition &definition : definitions) {
		std::vector<VehicleLandmarkObservation> landmark_observations;
		for (const VehicleViewObservation &view : observations.observations()) {
			const auto projected = VehicleProjectionService().projectPoint(
				view.camera(), definition.position);
			if (projected.has_value()) {
				landmark_observations.emplace_back(
					view.camera().identifier(), *projected, 0.002f, 1.0f);
			}
		}
		landmarks.emplace_back(
			definition.identifier, definition.position,
			std::move(landmark_observations), definition.symmetry_partner,
			definition.topology_role, VehicleEvidenceClassification::Observed,
			VehicleConstraintStrength::Soft);
	}
	return landmarks;
}

std::vector<AutomotiveWireframeEdge> create_wireframe_edges()
{
	return {
		{"Centre.Hood", "HoodFrontCentre", "HoodRearCentre", AutomotiveWireframeEdgeRole::CentreSpine},
		{"Centre.RoofFront", "HoodRearCentre", "RoofFrontCentre", AutomotiveWireframeEdgeRole::CentreSpine},
		{"Centre.RoofApex", "RoofFrontCentre", "RoofApex", AutomotiveWireframeEdgeRole::CentreSpine},
		{"Centre.RoofRear", "RoofApex", "RoofRearCentre", AutomotiveWireframeEdgeRole::CentreSpine},
		{"RoofRail.L.Front", "APillarRoofL", "BPillarRoofL", AutomotiveWireframeEdgeRole::RoofRail},
		{"RoofRail.L.Rear", "BPillarRoofL", "CPillarRoofL", AutomotiveWireframeEdgeRole::RoofRail},
		{"RoofRail.R.Front", "APillarRoofR", "BPillarRoofR", AutomotiveWireframeEdgeRole::RoofRail},
		{"RoofRail.R.Rear", "BPillarRoofR", "CPillarRoofR", AutomotiveWireframeEdgeRole::RoofRail},
		{"Belt.L.Front", "APillarBaseL", "BPillarBeltL", AutomotiveWireframeEdgeRole::BeltRail},
		{"Belt.L.Rear", "BPillarBeltL", "CPillarBeltL", AutomotiveWireframeEdgeRole::BeltRail},
		{"Belt.R.Front", "APillarBaseR", "BPillarBeltR", AutomotiveWireframeEdgeRole::BeltRail},
		{"Belt.R.Rear", "BPillarBeltR", "CPillarBeltR", AutomotiveWireframeEdgeRole::BeltRail},
		{"Wheelbase.L", "FrontWheelCentreL", "RearWheelCentreL", AutomotiveWireframeEdgeRole::WheelBase},
		{"Wheelbase.R", "FrontWheelCentreR", "RearWheelCentreR", AutomotiveWireframeEdgeRole::WheelBase},
		{"Glass.Front.L", "APillarBaseL", "APillarRoofL", AutomotiveWireframeEdgeRole::GlassPerimeter},
		{"Glass.Front.R", "APillarBaseR", "APillarRoofR", AutomotiveWireframeEdgeRole::GlassPerimeter},
		{"Door.Front.L", "APillarBaseL", "BPillarBeltL", AutomotiveWireframeEdgeRole::DoorAperture},
		{"Door.Front.R", "APillarBaseR", "BPillarBeltR", AutomotiveWireframeEdgeRole::DoorAperture},
		{"Door.Rear.L", "BPillarBeltL", "CPillarBeltL", AutomotiveWireframeEdgeRole::DoorAperture},
		{"Door.Rear.R", "BPillarBeltR", "CPillarBeltR", AutomotiveWireframeEdgeRole::DoorAperture},
		{"Bumper.Front", "HeadlampOuterL", "HeadlampOuterR", AutomotiveWireframeEdgeRole::BumperBound},
		{"Bumper.Rear", "TailLampOuterL", "TailLampOuterR", AutomotiveWireframeEdgeRole::BumperBound}};
}

std::vector<SilhouetteConstraint> create_silhouette_constraints(
	const VehicleObservationSet &observations)
{
	const std::vector<glm::vec3> model_points = {
		{-0.90f, 0.12f, 2.175f}, {0.90f, 0.12f, 2.175f},
		{0.90f, 0.90f, 1.25f}, {0.65f, 1.43f, -0.40f},
		{0.58f, 1.27f, -1.65f}, {0.86f, 0.62f, -2.175f},
		{-0.86f, 0.62f, -2.175f}, {-0.58f, 1.27f, -1.65f},
		{-0.65f, 1.43f, -0.40f}, {-0.90f, 0.90f, 1.25f},
		{-0.90f, 0.12f, 2.175f}};
	std::vector<SilhouetteConstraint> constraints;
	for (const VehicleViewObservation &view : observations.observations()) {
		constraints.emplace_back(
			"Silhouette." + view.camera().identifier(), view.camera().identifier(),
			model_points, view.silhouette(), 1.0f, VehicleConstraintStrength::Soft,
			0.002f);
	}
	return constraints;
}

struct PatchDefinition {
	const char *identifier;
	glm::vec3 minimum;
	glm::vec3 maximum;
	float lateral_offset;
};

ClassASurfaceGraph create_class_a_surface_graph(
	std::vector<AutomotiveCharacterCurve> semantic_curves)
{
	const std::vector<PatchDefinition> patch_definitions = {
		{"HoodCentre", {-0.28f, 0.82f, 0.82f}, {0.28f, 0.94f, 2.08f}, 0.0f},
		{"HoodShoulderL", {-0.72f, 0.79f, 0.82f}, {-0.28f, 0.92f, 2.05f}, -0.02f},
		{"HoodShoulderR", {0.28f, 0.79f, 0.82f}, {0.72f, 0.92f, 2.05f}, 0.02f},
		{"FrontFenderL", {-0.90f, 0.36f, 0.75f}, {-0.70f, 0.82f, 1.90f}, -0.03f},
		{"FrontFenderR", {0.70f, 0.36f, 0.75f}, {0.90f, 0.82f, 1.90f}, 0.03f},
		{"FrontDoorSkinL", {-0.90f, 0.22f, -0.20f}, {-0.82f, 0.88f, 0.80f}, -0.03f},
		{"FrontDoorSkinR", {0.82f, 0.22f, -0.20f}, {0.90f, 0.88f, 0.80f}, 0.03f},
		{"RearDoorSkinL", {-0.90f, 0.22f, -1.25f}, {-0.82f, 0.88f, -0.20f}, -0.03f},
		{"RearDoorSkinR", {0.82f, 0.22f, -1.25f}, {0.90f, 0.88f, -0.20f}, 0.03f},
		{"RoofCentre", {-0.30f, 1.34f, -1.45f}, {0.30f, 1.43f, 0.35f}, 0.0f},
		{"RoofRailL", {-0.66f, 1.28f, -1.42f}, {-0.30f, 1.39f, 0.35f}, -0.02f},
		{"RoofRailR", {0.30f, 1.28f, -1.42f}, {0.66f, 1.39f, 0.35f}, 0.02f},
		{"QuarterPanelL", {-0.88f, 0.34f, -2.05f}, {-0.58f, 1.26f, -1.20f}, -0.03f},
		{"QuarterPanelR", {0.58f, 0.34f, -2.05f}, {0.88f, 1.26f, -1.20f}, 0.03f},
		{"Tailgate", {-0.70f, 0.45f, -2.16f}, {0.70f, 1.25f, -1.48f}, 0.0f},
		{"FrontBumper", {-0.88f, 0.18f, 1.90f}, {0.88f, 0.72f, 2.17f}, 0.0f},
		{"RearBumper", {-0.86f, 0.18f, -2.17f}, {0.86f, 0.68f, -1.90f}, 0.0f}};
	std::vector<ClassASurfacePatch> patches;
	for (const PatchDefinition &definition : patch_definitions) {
		const float x0 = definition.minimum.x;
		const float x1 = definition.maximum.x;
		const float z0 = definition.minimum.z;
		const float z1 = definition.maximum.z;
		const float y0 = definition.minimum.y;
		const float y1 = definition.maximum.y;
		const std::vector<glm::vec3> grid = {
			{x0, y0, z0}, {x1, y0 + definition.lateral_offset, z0},
			{x0, y1, z1}, {x1, y1 + definition.lateral_offset, z1}};
		const std::string name = definition.identifier;
		const std::array<std::string, 4> boundaries = {
			name + ".U0", name + ".U1", name + ".V0", name + ".V1"};
		const std::vector<std::string> internal_guides = {
			name + ".Guide.UCenter", name + ".Guide.VCenter"};
		semantic_curves.emplace_back(
			boundaries[0], name + ".Boundary", 1,
			std::vector<glm::vec3>{grid[0], grid[2]},
			std::vector<std::string>{}, std::vector<std::string>{});
		semantic_curves.emplace_back(
			boundaries[1], name + ".Boundary", 1,
			std::vector<glm::vec3>{grid[1], grid[3]},
			std::vector<std::string>{}, std::vector<std::string>{});
		semantic_curves.emplace_back(
			boundaries[2], name + ".Boundary", 1,
			std::vector<glm::vec3>{grid[0], grid[1]},
			std::vector<std::string>{}, std::vector<std::string>{});
		semantic_curves.emplace_back(
			boundaries[3], name + ".Boundary", 1,
			std::vector<glm::vec3>{grid[2], grid[3]},
			std::vector<std::string>{}, std::vector<std::string>{});
		semantic_curves.emplace_back(
			internal_guides[0], name + ".Guide", 1,
			std::vector<glm::vec3>{glm::mix(grid[0], grid[1], 0.5f),
				glm::mix(grid[2], grid[3], 0.5f)},
			std::vector<std::string>{}, std::vector<std::string>{});
		semantic_curves.emplace_back(
			internal_guides[1], name + ".Guide", 1,
			std::vector<glm::vec3>{glm::mix(grid[0], grid[2], 0.5f),
				glm::mix(grid[1], grid[3], 0.5f)},
			std::vector<std::string>{}, std::vector<std::string>{});
		patches.emplace_back(
			name, boundaries, internal_guides, 1, 1, 1, 1,
			2u, 2u, grid);
	}
	const std::array<PatchContinuityLevel, 7> levels = {
		PatchContinuityLevel::G0, PatchContinuityLevel::G1,
		PatchContinuityLevel::G2, PatchContinuityLevel::G3,
		PatchContinuityLevel::Crease, PatchContinuityLevel::PanelGap,
		PatchContinuityLevel::Trimmed};
	std::vector<PatchConnection> connections;
	for (std::size_t index = 1u; index < patches.size(); ++index) {
		connections.emplace_back(
			"Connection." + patches[index - 1u].identifier() + "." +
				patches[index].identifier(),
			patches[index - 1u].identifier(), "U1",
			patches[index].identifier(), "U0", levels[(index - 1u) % levels.size()],
			0.005f);
	}
	return ClassASurfaceGraph(
		std::move(semantic_curves), std::move(patches), std::move(connections));
}

Curve3D create_closed_rectangle_curve(
	const std::string &identifier,
	const AxisAlignedBounds &bounds,
	float lateral_position)
{
	return Curve3D(
		Curve3DType::Polyline,
		{{lateral_position, bounds.min.y, bounds.max.z},
		 {lateral_position, bounds.max.y, bounds.max.z},
		 {lateral_position, bounds.max.y, bounds.min.z},
		 {lateral_position, bounds.min.y, bounds.min.z},
		 {lateral_position, bounds.min.y, bounds.max.z}},
		identifier);
}

std::vector<AutomotivePanelGap> create_panel_gaps(
	const std::vector<std::pair<std::string, AxisAlignedBounds>> &closure_bounds)
{
	std::vector<AutomotivePanelGap> gaps;
	for (const auto &entry : closure_bounds) {
		const float lateral = entry.second.center.x;
		Curve3D seam = create_closed_rectangle_curve(
			entry.first + ".Seam", entry.second, lateral);
		gaps.emplace_back(
			entry.first + ".PanelGap", seam,
			std::vector<AutomotiveScalarStation>{{0.0f, 0.0038f}, {0.5f, 0.0042f}, {1.0f, 0.0038f}},
			std::vector<AutomotiveScalarStation>{{0.0f, 0.0025f}, {0.5f, 0.0030f}, {1.0f, 0.0025f}},
			std::vector<AutomotiveScalarStation>{{0.0f, 0.0018f}, {0.5f, 0.0022f}, {1.0f, 0.0018f}},
			AutomotiveFlange(entry.first + ".PrimaryFlange", seam.identifier(), 0.014f, 82.0f),
			AutomotiveFlange(entry.first + ".SecondaryFlange", seam.identifier(), 0.012f, 96.0f),
			PatchContinuityLevel::G2, PatchContinuityLevel::G1, true);
	}
	return gaps;
}

std::vector<RolledEdge> create_rolled_edges(
	const std::vector<std::pair<std::string, AxisAlignedBounds>> &closure_bounds)
{
	std::vector<RolledEdge> edges;
	for (const auto &entry : closure_bounds) {
		edges.emplace_back(
			entry.first + ".RolledEdge",
			create_closed_rectangle_curve(
				entry.first + ".RolledContact", entry.second, entry.second.center.x),
			0.0030f, 0.016f, 88.0f, PatchContinuityLevel::G2);
	}
	return edges;
}

BodySideAperture create_aperture(
	const std::string &identifier,
	const AxisAlignedBounds &bounds,
	const std::string &side)
{
	return BodySideAperture(
		identifier, identifier + ".HingePillar", "A.Pillar." + side,
		"RoofRail." + side, "B.Pillar." + side, "Rocker." + side,
		"C.Pillar." + side, identifier + ".Dogleg",
		create_closed_rectangle_curve(identifier + ".BLine", bounds, bounds.center.x),
		identifier + ".JSurface", identifier + ".SideGlass",
		identifier + ".Flange", identifier + ".SealSeat");
}

ClosureHingeStudy create_hinge_study(
	const std::string &identifier,
	glm::vec3 lower_hinge,
	glm::vec3 upper_hinge,
	float maximum_degrees,
	const AxisAlignedBounds &source_bounds)
{
	AutomotiveClosureGeometryService geometry;
	ClosureHingeStudy provisional(
		identifier, upper_hinge, lower_hinge, 0.0f, maximum_degrees,
		{}, source_bounds, source_bounds);
	std::vector<glm::vec2> rise_curve;
	for (float state : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
		rise_curve.emplace_back(state, geometry.calculateClosureRise(provisional, state));
	}
	return ClosureHingeStudy(
		identifier, upper_hinge, lower_hinge, 0.0f, maximum_degrees,
		std::move(rise_curve), source_bounds,
		geometry.calculateClosureSweptBounds(provisional, 17u));
}

std::vector<AutomotiveClosure> create_closures(
	const std::vector<std::pair<std::string, AxisAlignedBounds>> &closure_bounds)
{
	std::map<std::string, AxisAlignedBounds> bounds;
	for (const auto &entry : closure_bounds) bounds[entry.first] = entry.second;
	std::vector<AutomotiveClosure> closures;
	const struct DoorDefinition {
		const char *identifier;
		AutomotiveClosureType type;
		const char *patch;
		const char *aperture;
		const char *j_surface;
		const char *glass;
		glm::vec3 lower_hinge;
		glm::vec3 upper_hinge;
	} doors[] = {
		{"DoorFL", AutomotiveClosureType::FrontDoor, "FrontDoorSkinL", "ApertureFL", "ApertureFL.JSurface", "GlassFL", {-0.87f, 0.40f, 0.72f}, {-0.87f, 1.02f, 0.72f}},
		{"DoorFR", AutomotiveClosureType::FrontDoor, "FrontDoorSkinR", "ApertureFR", "ApertureFR.JSurface", "GlassFR", {0.87f, 0.40f, 0.72f}, {0.87f, 1.02f, 0.72f}},
		{"DoorRL", AutomotiveClosureType::RearDoor, "RearDoorSkinL", "ApertureRL", "ApertureRL.JSurface", "GlassRL", {-0.87f, 0.40f, -0.24f}, {-0.87f, 1.02f, -0.24f}},
		{"DoorRR", AutomotiveClosureType::RearDoor, "RearDoorSkinR", "ApertureRR", "ApertureRR.JSurface", "GlassRR", {0.87f, 0.40f, -0.24f}, {0.87f, 1.02f, -0.24f}}};
	for (const DoorDefinition &door : doors) {
		closures.emplace_back(
			door.identifier, door.type, door.patch, door.aperture, door.j_surface,
			door.glass,
			create_hinge_study(
				std::string(door.identifier) + ".HingeStudy", door.lower_hinge,
				door.upper_hinge, 68.0f, bounds[door.identifier]),
			std::vector<std::string>{std::string(door.identifier) + ".InnerPanel",
				std::string(door.identifier) + ".CrashBeam"},
			bounds[door.identifier]);
	}
	closures.emplace_back(
		"Bonnet", AutomotiveClosureType::Bonnet, "HoodCentre", "Bonnet.Aperture",
		"Bonnet.InnerPanel", "",
		create_hinge_study(
			"Bonnet.HingeStudy", {-0.58f, 0.93f, 0.82f},
			{0.58f, 0.93f, 0.82f}, 68.0f, bounds["Bonnet"]),
		std::vector<std::string>{"Bonnet.InnerPanel", "Bonnet.LatchReinforcement"},
		bounds["Bonnet"]);
	closures.emplace_back(
		"RearHatch", AutomotiveClosureType::RearHatch, "Tailgate",
		"RearHatch.Aperture", "RearHatch.InnerPanel", "RearGlass",
		create_hinge_study(
			"RearHatch.HingeStudy", {-0.58f, 1.26f, -1.48f},
			{0.58f, 1.26f, -1.48f}, 82.0f, bounds["RearHatch"]),
		std::vector<std::string>{"RearGlass", "RearSpoiler", "RearWiper",
			"HighMountedBrakeLamp"},
		bounds["RearHatch"]);
	return closures;
}

SideGlassSurface create_side_glass_surface(
	const std::string &identifier,
	const AxisAlignedBounds &bounds)
{
	return SideGlassSurface(
		identifier,
		{{bounds.center.x, bounds.min.y, bounds.max.z},
		 {bounds.center.x, bounds.max.y, bounds.max.z},
		 {bounds.center.x, bounds.max.y, bounds.min.z},
		 {bounds.center.x, bounds.min.y, bounds.min.z}},
		bounds);
}

std::vector<HelicalGlassDrop> create_glass_drops()
{
	const std::vector<std::tuple<std::string, std::string, AxisAlignedBounds, AxisAlignedBounds, float>> definitions = {
		{"GlassFL", "DoorFL", create_bounds({-0.895f, 0.72f, -0.12f}, {-0.865f, 1.18f, 0.68f}), create_bounds({-0.96f, 0.10f, -0.22f}, {-0.79f, 1.25f, 0.78f}), -4.0f},
		{"GlassFR", "DoorFR", create_bounds({0.865f, 0.72f, -0.12f}, {0.895f, 1.18f, 0.68f}), create_bounds({0.79f, 0.10f, -0.22f}, {0.96f, 1.25f, 0.78f}), 4.0f},
		{"GlassRL", "DoorRL", create_bounds({-0.895f, 0.70f, -1.20f}, {-0.865f, 1.16f, -0.28f}), create_bounds({-0.96f, 0.08f, -1.30f}, {-0.79f, 1.23f, -0.18f}), -3.5f},
		{"GlassRR", "DoorRR", create_bounds({0.865f, 0.70f, -1.20f}, {0.895f, 1.16f, -0.28f}), create_bounds({0.79f, 0.08f, -1.30f}, {0.96f, 1.23f, -0.18f}), 3.5f}};
	std::vector<HelicalGlassDrop> drops;
	AutomotiveClosureGeometryService geometry;
	for (const auto &definition : definitions) {
		const std::string &identifier = std::get<0>(definition);
		const AxisAlignedBounds &glass_bounds = std::get<2>(definition);
		SideGlassSurface surface = create_side_glass_surface(identifier, glass_bounds);
		GlassChannel front = geometry.generateChannel(
			identifier + ".FrontChannel", surface, true, 0.018f, 0.014f, 0.002f);
		GlassChannel rear = geometry.generateChannel(
			identifier + ".RearChannel", surface, false, 0.018f, 0.014f, 0.002f);
		drops.emplace_back(
			identifier + ".HelicalDrop", std::get<1>(definition), surface,
			BarrelSurface(
				identifier + ".Barrel", glass_bounds.center,
				{0.0f, -1.0f, 0.0f}, 8.0f, glass_bounds.max.y - glass_bounds.min.y),
			glm::vec3(0.0f, -1.0f, 0.0f), 0.52f, std::get<4>(definition),
			std::move(front), std::move(rear), 0.0f, 1.0f,
			std::get<3>(definition));
	}
	return drops;
}

std::vector<VariableSealSweep> create_seals(
	const std::vector<std::pair<std::string, AxisAlignedBounds>> &closure_bounds)
{
	std::vector<VariableSealSweep> seals;
	for (const auto &entry : closure_bounds) {
		seals.emplace_back(
			entry.first + ".VariableSeal",
			create_closed_rectangle_curve(
				entry.first + ".SealPath", entry.second, entry.second.center.x),
			std::vector<VariableSealSectionStation>{
				{0.0f, 0.018f, 0.022f, 0.28f},
				{0.5f, 0.020f, 0.025f, 0.32f},
				{1.0f, 0.018f, 0.022f, 0.28f}},
			"EPDM", std::vector<std::string>{entry.first + ".SealSeat",
				entry.first + ".RolledEdge"});
	}
	return seals;
}

AutomotiveStructuralMember create_structural_member(
	const std::string &identifier,
	const std::string &role,
	glm::vec3 start,
	glm::vec3 end)
{
	const glm::vec3 hole_centre = glm::mix(start, end, 0.5f);
	const glm::vec3 hole_half_extents(0.012f);
	return AutomotiveStructuralMember(
		identifier, role, Curve3D(Curve3DType::Line, {start, end}, identifier + ".Path"),
		{{0.0f, 0.08f, 0.11f, 0.0012f, 0.018f},
		 {0.5f, 0.10f, 0.12f, 0.0014f, 0.020f},
		 {1.0f, 0.08f, 0.10f, 0.0012f, 0.018f}},
		{create_bounds(hole_centre - hole_half_extents,
			hole_centre + hole_half_extents)},
		{identifier + ".LocalReinforcement"}, "DP600-Steel",
		"MVPv2.5 engineering inference constrained by package and aperture geometry");
}

BodyInWhite create_body_in_white()
{
	std::vector<AutomotiveStructuralMember> members = {
		create_structural_member("FrontRailL", "FrontLongitudinalRail", {-0.52f, 0.34f, 2.00f}, {-0.52f, 0.30f, 0.55f}),
		create_structural_member("FrontRailR", "FrontLongitudinalRail", {0.52f, 0.34f, 2.00f}, {0.52f, 0.30f, 0.55f}),
		create_structural_member("APillarL", "APillar", {-0.76f, 0.40f, 0.70f}, {-0.62f, 1.34f, 0.35f}),
		create_structural_member("APillarR", "APillar", {0.76f, 0.40f, 0.70f}, {0.62f, 1.34f, 0.35f}),
		create_structural_member("BPillarL", "BPillar", {-0.82f, 0.24f, -0.25f}, {-0.64f, 1.40f, -0.30f}),
		create_structural_member("BPillarR", "BPillar", {0.82f, 0.24f, -0.25f}, {0.64f, 1.40f, -0.30f}),
		create_structural_member("CPillarL", "CPillar", {-0.80f, 0.26f, -1.28f}, {-0.58f, 1.28f, -1.32f}),
		create_structural_member("CPillarR", "CPillar", {0.80f, 0.26f, -1.28f}, {0.58f, 1.28f, -1.32f}),
		create_structural_member("RoofRailL", "RoofSideRail", {-0.62f, 1.34f, 0.35f}, {-0.58f, 1.28f, -1.32f}),
		create_structural_member("RoofRailR", "RoofSideRail", {0.62f, 1.34f, 0.35f}, {0.58f, 1.28f, -1.32f}),
		create_structural_member("RockerL", "Rocker", {-0.82f, 0.22f, 0.70f}, {-0.80f, 0.22f, -1.35f}),
		create_structural_member("RockerR", "Rocker", {0.82f, 0.22f, 0.70f}, {0.80f, 0.22f, -1.35f}),
		create_structural_member("Firewall", "Firewall", {-0.70f, 0.28f, 0.70f}, {0.70f, 0.28f, 0.70f}),
		create_structural_member("ToePan", "ToePan", {-0.65f, 0.18f, 0.58f}, {0.65f, 0.18f, 0.58f}),
		create_structural_member("FloorCentre", "FloorPanelSupport", {0.0f, 0.16f, 0.55f}, {0.0f, 0.16f, -1.45f}),
		create_structural_member("FrontCrossmember", "FrontCrossmember", {-0.72f, 0.28f, 1.88f}, {0.72f, 0.28f, 1.88f}),
		create_structural_member("RearCrossmember", "RearCrossmember", {-0.76f, 0.24f, -1.45f}, {0.76f, 0.24f, -1.45f}),
		create_structural_member("RearStructure", "RearBodyStructure", {-0.70f, 0.28f, -1.45f}, {0.70f, 0.58f, -2.02f})};
	std::vector<BIWJoint> joints;
	for (std::size_t index = 1u; index < members.size(); ++index) {
		joints.emplace_back(
			"BIWJoint." + members[index - 1u].identifier() + "." + members[index].identifier(),
			std::vector<std::string>{members[index - 1u].identifier(), members[index].identifier()},
			std::vector<std::string>{members[index - 1u].identifier() + ".Overlap",
				members[index].identifier() + ".Overlap"},
			index % 3u == 0u ? BIWJoiningMethod::Mixed : BIWJoiningMethod::SpotWeld,
			"JointReinforcement." + std::to_string(index),
			glm::mix(
				members[index - 1u].centreLine().controlPoints().back(),
				members[index].centreLine().controlPoints().front(), 0.5f));
	}
	return BodyInWhite("MVP25.RedHatchback.BIW", std::move(members), std::move(joints));
}

SuspensionHardpointModel create_suspension_model(
	const std::string &identifier,
	VehicleCornerLocation corner,
	glm::vec3 wheel_centre,
	bool steering)
{
	const float side = wheel_centre.x < 0.0f ? -1.0f : 1.0f;
	std::vector<SuspensionHardpoint> hardpoints = {
		{identifier + ".WheelCentre", SuspensionHardpointRole::WheelCentre, corner,
		 wheel_centre, VehicleEvidenceClassification::Observed, 0.002f},
		{identifier + ".UpperBodyMount", SuspensionHardpointRole::UpperBodyMount, corner,
		 wheel_centre + glm::vec3(-0.10f * side, 0.42f, -0.02f),
		 VehicleEvidenceClassification::EngineeringInference, 0.005f},
		{identifier + ".LowerBodyMount", SuspensionHardpointRole::LowerBodyMount, corner,
		 wheel_centre + glm::vec3(-0.18f * side, -0.06f, 0.04f),
		 VehicleEvidenceClassification::EngineeringInference, 0.005f},
		{identifier + ".TieRodInner", SuspensionHardpointRole::TieRodInner, corner,
		 wheel_centre + glm::vec3(-0.24f * side, 0.02f, -0.14f),
		 VehicleEvidenceClassification::EngineeringInference, 0.005f},
		{identifier + ".TieRodOuter", SuspensionHardpointRole::TieRodOuter, corner,
		 wheel_centre + glm::vec3(-0.04f * side, 0.02f, -0.10f),
		 VehicleEvidenceClassification::EngineeringInference, 0.005f}};
	std::vector<KinematicLink> links;
	for (std::size_t index = 1u; index < hardpoints.size(); ++index) {
		links.emplace_back(
			identifier + ".Link." + std::to_string(index),
			hardpoints[0].identifier(), hardpoints[index].identifier(),
			glm::length(hardpoints[0].position() - hardpoints[index].position()));
	}
	return SuspensionHardpointModel(
		identifier, corner, std::move(hardpoints), std::move(links),
		-0.08f, 0.08f, steering ? -32.0f : 0.0f, steering ? 32.0f : 0.0f);
}

std::vector<TyreGeometry> create_tyres()
{
	std::vector<TyreGeometry> tyres;
	for (const char *corner : {"FL", "FR", "RL", "RR"}) {
		tyres.emplace_back(
			std::string("Tyre.") + corner, 0.225f, 0.45f, 0.225f,
			0.36f, 0.055f, 0.012f, 0.008f, 0.33f, 0.315f);
	}
	return tyres;
}

std::vector<AutomotiveRimGeometry> create_rim_geometries()
{
	return {
		{"Rim.FL", VehicleCornerLocation::FrontLeft,
		 0.225f, 0.205f, 0.205f, 10u, "ForgedAluminium"},
		{"Rim.FR", VehicleCornerLocation::FrontRight,
		 0.225f, 0.205f, 0.205f, 10u, "ForgedAluminium"},
		{"Rim.RL", VehicleCornerLocation::RearLeft,
		 0.225f, 0.205f, 0.205f, 10u, "ForgedAluminium"},
		{"Rim.RR", VehicleCornerLocation::RearRight,
		 0.225f, 0.205f, 0.205f, 10u, "ForgedAluminium"},
	};
}

std::vector<AutomotiveBrakeGeometry> create_brake_geometries()
{
	return {
		{"Brake.FL", VehicleCornerLocation::FrontLeft,
		 0.155f, 0.030f,
		 create_bounds({-0.805f, 0.29f, 1.18f}, {-0.695f, 0.47f, 1.28f}),
		 4u, "HighCarbonCastIron"},
		{"Brake.FR", VehicleCornerLocation::FrontRight,
		 0.155f, 0.030f,
		 create_bounds({0.695f, 0.29f, 1.18f}, {0.805f, 0.47f, 1.28f}),
		 4u, "HighCarbonCastIron"},
		{"Brake.RL", VehicleCornerLocation::RearLeft,
		 0.145f, 0.024f,
		 create_bounds({-0.805f, 0.29f, -1.47f}, {-0.695f, 0.45f, -1.37f}),
		 2u, "HighCarbonCastIron"},
		{"Brake.RR", VehicleCornerLocation::RearRight,
		 0.145f, 0.024f,
		 create_bounds({0.695f, 0.29f, -1.47f}, {0.805f, 0.45f, -1.37f}),
		 2u, "HighCarbonCastIron"},
	};
}

AeroGeometry create_aero_geometry()
{
	return AeroGeometry({
		{"Aero.UpperBody", AeroGeometryRole::UpperBody, create_bounds({-0.90f, 0.12f, -2.175f}, {0.90f, 1.43f, 2.175f}), 1.0f},
		{"Aero.Wheels", AeroGeometryRole::Wheel, create_bounds({-0.90f, -0.02f, -1.70f}, {0.90f, 0.70f, 1.70f}), 0.8f},
		{"Aero.WheelHouses", AeroGeometryRole::WheelHouse, create_bounds({-0.96f, -0.10f, -1.75f}, {0.96f, 0.82f, 1.75f}), 0.7f},
		{"Aero.Underbody", AeroGeometryRole::Underbody, create_bounds({-0.78f, 0.10f, -1.80f}, {0.78f, 0.20f, 1.85f}), 0.9f},
		{"Aero.Splitter", AeroGeometryRole::Splitter, create_bounds({-0.74f, 0.12f, 2.08f}, {0.74f, 0.18f, 2.20f}), 0.5f},
		{"Aero.CoolingOpening", AeroGeometryRole::CoolingOpening, create_bounds({-0.52f, 0.30f, 2.14f}, {0.52f, 0.56f, 2.18f}), 0.6f},
		{"Aero.Diffuser", AeroGeometryRole::Diffuser, create_bounds({-0.68f, 0.12f, -2.20f}, {0.68f, 0.32f, -1.82f}), 0.6f},
		{"Aero.Spoiler", AeroGeometryRole::Spoiler, create_bounds({-0.65f, 1.26f, -1.78f}, {0.65f, 1.34f, -1.58f}), 0.5f}});
}

VehicleFitObjective create_fit_objective()
{
	return VehicleFitObjective(
		{{VehicleFitResidualTerm::Landmark, 1.0f},
		 {VehicleFitResidualTerm::Silhouette, 1.0f},
		 {VehicleFitResidualTerm::CharacterCurve, 0.8f},
		 {VehicleFitResidualTerm::Package, 4.0f},
		 {VehicleFitResidualTerm::GapClosure, 0.7f},
		 {VehicleFitResidualTerm::Kinematic, 1.2f},
		 {VehicleFitResidualTerm::ClassA, 0.9f},
		 {VehicleFitResidualTerm::ShapePrior, 0.2f},
		 {VehicleFitResidualTerm::Complexity, 0.05f}},
		{{"Hard.Length", VehicleFitResidualTerm::Package, VehicleConstraintStrength::Hard, 1.0e-6f, 0.0f},
		 {"Hard.Width", VehicleFitResidualTerm::Package, VehicleConstraintStrength::Hard, 1.0e-6f, 0.0f},
		 {"Hard.Height", VehicleFitResidualTerm::Package, VehicleConstraintStrength::Hard, 1.0e-6f, 0.0f},
		 {"Hard.Wheelbase", VehicleFitResidualTerm::Package, VehicleConstraintStrength::Hard, 1.0e-6f, 0.0f},
		 {"Soft.HatchbackPrior", VehicleFitResidualTerm::ShapePrior, VehicleConstraintStrength::Soft, 0.10f, 0.01f}},
		0.01f, 0.0002f, 0.002f);
}

} // namespace

VehicleMvp25Architecture RedAwdHatchbackMvp25Builder::createArchitecture() const
{
	const VehicleReferenceFrame reference_frame(
		VehicleReferenceConvention::SaeDesignIntent, {0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
		0.0f, 0.0f, 0.0f, 0.0f,
		{{"FrontAxleDatum", {0.0f, 0.32f, 1.325f}, {1.0f, 0.0f, 0.0f}},
		 {"RearAxleDatum", {0.0f, 0.32f, -1.325f}, {1.0f, 0.0f, 0.0f}},
		 {"VehicleCentrePlane", {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}});
	const VehiclePackageEvidence package(
		4.350f, 1.800f, 1.430f, 2.650f, 1.500f, 1.500f,
		0.850f, 0.850f, {0.0f, 0.32f, 1.325f},
		{0.0f, 0.32f, -1.325f}, VehicleDriveEvidence::AllWheelDrive, 1.0e-5f);
	const std::vector<OccupantPackage> occupants = {
		{"Occupant.Front", {0.32f, 0.56f, 0.28f}, {0.32f, 1.16f, 0.38f},
		 {0.32f, 0.14f, 0.72f}, create_bounds({0.10f, 0.36f, 0.42f}, {0.54f, 0.78f, 0.94f}),
		 create_bounds({0.08f, 0.96f, 0.06f}, {0.56f, 1.36f, 0.58f}),
		 {0.0f, 0.94f, 0.34f}, create_bounds({-0.10f, 0.46f, 0.10f}, {0.74f, 1.22f, 1.08f})},
		{"Occupant.Rear", {0.30f, 0.58f, -0.72f}, {0.30f, 1.14f, -0.64f},
		 {0.30f, 0.16f, -0.12f}, create_bounds({0.08f, 0.34f, -0.54f}, {0.52f, 0.78f, -0.02f}),
		 create_bounds({0.06f, 0.94f, -0.98f}, {0.54f, 1.34f, -0.48f}),
		 {0.0f, 0.96f, 0.28f}, create_bounds({-0.08f, 0.46f, -1.08f}, {0.70f, 1.20f, -0.05f})}};

	const std::vector<VehicleCameraModel> cameras = create_cameras();
	std::vector<AutomotiveCharacterCurve> semantic_curves = create_semantic_character_curves();
	const std::vector<LandmarkDefinition> landmark_definitions = create_landmark_definitions();
	VehicleObservationSet observations = create_observation_set(
		cameras, landmark_definitions, semantic_curves);
	std::vector<VehicleLandmark> landmarks = create_landmarks(
		landmark_definitions, observations);
	AutomotiveWireframe wireframe(
		"MVP25.RedHatchback.Wireframe", std::move(landmarks),
		create_wireframe_edges(),
		VehicleShapePrior(
			"SportCompactHatchback", "FiveDoorHatchbackTopology", 0.20f,
			VehicleConstraintStrength::Soft));
	std::vector<SilhouetteConstraint> silhouettes =
		create_silhouette_constraints(observations);
	ClassASurfaceGraph class_a = create_class_a_surface_graph(
		std::move(semantic_curves));
	HighlightFlowReport highlight = HighlightFlowValidator().validate(class_a);

	const std::vector<std::pair<std::string, AxisAlignedBounds>> closure_bounds = {
		{"DoorFL", create_bounds({-0.90f, 0.22f, -0.18f}, {-0.82f, 1.18f, 0.78f})},
		{"DoorFR", create_bounds({0.82f, 0.22f, -0.18f}, {0.90f, 1.18f, 0.78f})},
		{"DoorRL", create_bounds({-0.90f, 0.22f, -1.25f}, {-0.82f, 1.16f, -0.22f})},
		{"DoorRR", create_bounds({0.82f, 0.22f, -1.25f}, {0.90f, 1.16f, -0.22f})},
		{"Bonnet", create_bounds({-0.76f, 0.78f, 0.82f}, {0.76f, 1.00f, 2.10f})},
		{"RearHatch", create_bounds({-0.72f, 0.46f, -2.16f}, {0.72f, 1.27f, -1.46f})}};
	std::vector<AutomotivePanelGap> panel_gaps = create_panel_gaps(closure_bounds);
	std::vector<RolledEdge> rolled_edges = create_rolled_edges(closure_bounds);
	std::vector<BodySideAperture> apertures = {
		create_aperture("ApertureFL", closure_bounds[0].second, "L"),
		create_aperture("ApertureFR", closure_bounds[1].second, "R"),
		create_aperture("ApertureRL", closure_bounds[2].second, "L"),
		create_aperture("ApertureRR", closure_bounds[3].second, "R")};
	std::vector<DoorEgressSurface> egress_surfaces;
	for (const BodySideAperture &aperture : apertures) {
		egress_surfaces.emplace_back(
			aperture.identifier() + ".EgressSurface", aperture.bLine().identifier(),
			aperture.glassSurfaceIdentifier(), aperture.jSurfaceIdentifier(),
			"BeltLine", 0.010f, 0.018f);
	}
	std::vector<AutomotiveClosure> closures = create_closures(closure_bounds);
	std::vector<HelicalGlassDrop> glass_drops = create_glass_drops();
	std::vector<VariableSealSweep> seals = create_seals(closure_bounds);
	BodyInWhite body_in_white = create_body_in_white();

	std::vector<SuspensionHardpointModel> suspension_models = {
		create_suspension_model("Suspension.FL", VehicleCornerLocation::FrontLeft, {-0.75f, 0.32f, 1.325f}, true),
		create_suspension_model("Suspension.FR", VehicleCornerLocation::FrontRight, {0.75f, 0.32f, 1.325f}, true),
		create_suspension_model("Suspension.RL", VehicleCornerLocation::RearLeft, {-0.75f, 0.32f, -1.325f}, false),
		create_suspension_model("Suspension.RR", VehicleCornerLocation::RearRight, {0.75f, 0.32f, -1.325f}, false)};
	std::vector<TyreGeometry> tyres = create_tyres();
	std::vector<AutomotiveRimGeometry> rims = create_rim_geometries();
	std::vector<AutomotiveBrakeGeometry> brakes = create_brake_geometries();
	std::vector<WheelPoseFunction> wheel_pose_functions;
	std::vector<WheelSweptEnvelope> wheel_envelopes;
	for (std::size_t index = 0u; index < suspension_models.size(); ++index) {
		const bool steering = index < 2u;
		WheelPoseFunction function = VehicleChassisKinematicsService().sampleWheelPoseFunction(
			suspension_models[index], 5u, steering ? 5u : 1u);
		wheel_envelopes.push_back(
			VehicleChassisKinematicsService().calculateWheelSweptEnvelope(
				function, tyres[index]));
		wheel_pose_functions.push_back(std::move(function));
	}
	std::vector<WheelHouse> wheel_houses = {
		{"WheelHouse.FL", VehicleCornerLocation::FrontLeft, create_bounds({-0.94f, -0.12f, 0.90f}, {-0.55f, 0.78f, 1.75f}), 0.01f},
		{"WheelHouse.FR", VehicleCornerLocation::FrontRight, create_bounds({0.55f, -0.12f, 0.90f}, {0.94f, 0.78f, 1.75f}), 0.01f},
		{"WheelHouse.RL", VehicleCornerLocation::RearLeft, create_bounds({-0.94f, -0.12f, -1.75f}, {-0.55f, 0.78f, -0.90f}), 0.01f},
		{"WheelHouse.RR", VehicleCornerLocation::RearRight, create_bounds({0.55f, -0.12f, -1.75f}, {0.94f, 0.78f, -0.90f}), 0.01f}};

	VehicleFitObjective objective = create_fit_objective();
	float landmark_error = 0.0f;
	for (const VehicleLandmark &landmark : wireframe.landmarks()) {
		landmark_error += VehicleProjectionService().calculateLandmarkError(
			landmark, observations);
	}
	landmark_error /= static_cast<float>(wireframe.landmarks().size());
	float silhouette_error = 0.0f;
	for (const SilhouetteConstraint &constraint : silhouettes) {
		silhouette_error += VehicleProjectionService().calculateSilhouetteError(
			constraint, observations);
	}
	silhouette_error /= static_cast<float>(silhouettes.size());
	float character_error = 0.0f;
	std::size_t observed_character_count = 0u;
	for (const AutomotiveCharacterCurve &curve : class_a.curves()) {
		if (curve.observationIdentifiers().empty()) continue;
		character_error += VehicleProjectionService().calculateCharacterLineError(
			curve, observations);
		++observed_character_count;
	}
	character_error /= static_cast<float>(observed_character_count);
	float class_a_error = 0.0f;
	for (const HighlightFlowResidual &residual : highlight.residuals()) {
		class_a_error += residual.positionError() + residual.tangentError() +
			residual.curvatureError() + residual.normalFlowError();
	}
	class_a_error /= static_cast<float>(highlight.residuals().size());
	const float complexity = VehicleClassAValidationService().calculateComplexityPenalty(
		highlight, objective.patchCountPenalty(), objective.controlPointPenalty(),
		objective.spanPenalty());
	VehicleFitResidualReport residual = VehicleFitObjectiveService().calculate(
		objective, VehicleFitMeasurementSet(
			landmark_error, silhouette_error, character_error, 0.0f, 0.0f,
			0.0f, class_a_error, 0.01f, complexity));

	return VehicleMvp25Architecture(
		"MVP25.RedAwdHatchback", reference_frame, package, occupants,
		std::move(observations), std::move(wireframe), std::move(silhouettes),
		std::move(class_a), std::move(panel_gaps), std::move(rolled_edges),
		std::move(apertures), std::move(egress_surfaces), std::move(closures),
		std::move(glass_drops), std::move(seals), std::move(body_in_white),
		std::move(suspension_models), std::move(wheel_pose_functions),
		std::move(wheel_envelopes), std::move(tyres), std::move(rims),
		std::move(brakes), std::move(wheel_houses),
		create_aero_geometry(), std::move(objective), std::move(residual),
		std::move(highlight));
}

std::optional<ModernVehicleAssembly> RedAwdHatchbackMvp25Builder::build(
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	std::optional<ModernVehicleAssembly> base_vehicle =
		RedAwdHatchbackMvpv2Builder().build(detail_level, diagnostic);
	if (!base_vehicle.has_value()) return std::nullopt;
	std::shared_ptr<const VehicleMvp25Architecture> architecture =
		std::make_shared<const VehicleMvp25Architecture>(createArchitecture());
	const VehicleValidationReport validation =
		VehicleMvp25ValidationService().validate(*architecture);
	if (!validation.isValid()) {
		if (diagnostic != nullptr) {
			*diagnostic = std::string(vehicleDiagnosticCodeName(
				validation.issues().front().code())) + ": " +
				validation.issues().front().message();
		}
		return std::nullopt;
	}
	const std::uint64_t hash =
		VehicleMvp25DeterministicHashService().calculate(*architecture);
	return base_vehicle->withMvp25Architecture(
		std::move(architecture), hash, validation);
}
