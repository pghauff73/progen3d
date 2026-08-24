#include "vehicle/service/VehicleDatumDerivationService.h"

#include <vector>

VehicleDatumSet VehicleDatumDerivationService::derive(
	const VehiclePackage &package,
	const VehicleStyleState &style) const
{
	const float front_wheel_y = package.frontWheelRadius();
	const float rear_wheel_y = package.rearWheelRadius();
	const float rear_axle_z = package.rearAxleStation();
	const float beltline_y = package.overallHeight() *
		(0.54f + 0.08f * style.beltlineSlope());
	const float rocker_y = package.groundClearance() + 0.12f;
	const float hood_y = package.overallHeight() * style.hoodHeightRatio();
	std::vector<VehicleDatum> datums;
	datums.reserve(14u);
	datums.emplace_back(
		VehicleDatumType::VehicleCentreline, glm::vec3(0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f), "VehicleCentreline");
	datums.emplace_back(
		VehicleDatumType::GroundPlane, glm::vec3(0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), "GroundPlane");
	datums.emplace_back(
		VehicleDatumType::FrontAxlePlane, glm::vec3(0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f), "FrontAxlePlane");
	datums.emplace_back(
		VehicleDatumType::RearAxlePlane, glm::vec3(0.0f, 0.0f, rear_axle_z),
		glm::vec3(0.0f, 0.0f, 1.0f), "RearAxlePlane");
	datums.emplace_back(
		VehicleDatumType::FrontWheelCentreLeft,
		glm::vec3(-package.frontTrack() * 0.5f, front_wheel_y, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f), "FrontWheelCentreLeft");
	datums.emplace_back(
		VehicleDatumType::FrontWheelCentreRight,
		glm::vec3(package.frontTrack() * 0.5f, front_wheel_y, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f), "FrontWheelCentreRight");
	datums.emplace_back(
		VehicleDatumType::RearWheelCentreLeft,
		glm::vec3(-package.rearTrack() * 0.5f, rear_wheel_y, rear_axle_z),
		glm::vec3(1.0f, 0.0f, 0.0f), "RearWheelCentreLeft");
	datums.emplace_back(
		VehicleDatumType::RearWheelCentreRight,
		glm::vec3(package.rearTrack() * 0.5f, rear_wheel_y, rear_axle_z),
		glm::vec3(1.0f, 0.0f, 0.0f), "RearWheelCentreRight");
	datums.emplace_back(
		VehicleDatumType::BeltLine, glm::vec3(0.0f, beltline_y, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), "BeltLine");
	datums.emplace_back(
		VehicleDatumType::RockerLine, glm::vec3(0.0f, rocker_y, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f), "RockerLine");
	datums.emplace_back(
		VehicleDatumType::RoofDatum,
		glm::vec3(0.0f, package.overallHeight(), -package.wheelbase() * 0.52f),
		glm::vec3(0.0f, 1.0f, 0.0f), "RoofDatum");
	datums.emplace_back(
		VehicleDatumType::HoodDatum,
		glm::vec3(0.0f, hood_y, package.frontOverhang() * 0.25f),
		glm::vec3(0.0f, 1.0f, 0.0f), "HoodDatum");
	datums.emplace_back(
		VehicleDatumType::DashboardPlane,
		glm::vec3(0.0f, beltline_y, -package.wheelbase() * 0.28f),
		glm::vec3(0.0f, 0.0f, 1.0f), "DashboardPlane");
	datums.emplace_back(
		VehicleDatumType::SeatReferencePlane,
		glm::vec3(0.0f, package.groundClearance() + 0.38f,
		          -package.wheelbase() * 0.47f),
		glm::vec3(0.0f, 0.0f, 1.0f), "SeatReferencePlane");
	return VehicleDatumSet(std::move(datums));
}
