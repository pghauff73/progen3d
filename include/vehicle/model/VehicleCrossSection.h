#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class VehicleSectionLandmarkRole
{
	CentreRoof,
	RoofRail,
	GlassShoulder,
	Belt,
	UpperShoulder,
	LowerDoor,
	Rocker,
	Underbody,
	CentreFloor
};

class VehicleSectionLandmark
{
public:
	VehicleSectionLandmark(VehicleSectionLandmarkRole role, glm::vec3 point)
		: role_(role), point_(point)
	{
	}

	VehicleSectionLandmarkRole role() const { return role_; }
	const glm::vec3 &point() const { return point_; }

private:
	VehicleSectionLandmarkRole role_ = VehicleSectionLandmarkRole::CentreRoof;
	glm::vec3 point_{0.0f};
};

class VehicleCrossSection
{
public:
	VehicleCrossSection(
		std::string identifier,
		float station_z,
		std::vector<VehicleSectionLandmark> landmarks)
		: identifier_(std::move(identifier)),
		  station_z_(station_z),
		  landmarks_(std::move(landmarks))
	{
	}

	const std::string &identifier() const { return identifier_; }
	float stationZ() const { return station_z_; }
	const std::vector<VehicleSectionLandmark> &landmarks() const
	{
		return landmarks_;
	}

	const VehicleSectionLandmark *findLandmark(
		VehicleSectionLandmarkRole role) const
	{
		for (const VehicleSectionLandmark &landmark : landmarks_) {
			if (landmark.role() == role) return &landmark;
		}
		return nullptr;
	}

	std::vector<glm::vec2> createRightHalfProfile() const
	{
		const VehicleSectionLandmarkRole ordered_roles[] = {
			VehicleSectionLandmarkRole::CentreFloor,
			VehicleSectionLandmarkRole::Underbody,
			VehicleSectionLandmarkRole::Rocker,
			VehicleSectionLandmarkRole::LowerDoor,
			VehicleSectionLandmarkRole::UpperShoulder,
			VehicleSectionLandmarkRole::Belt,
			VehicleSectionLandmarkRole::GlassShoulder,
			VehicleSectionLandmarkRole::RoofRail,
			VehicleSectionLandmarkRole::CentreRoof};
		std::vector<glm::vec2> profile;
		profile.reserve(9u);
		for (VehicleSectionLandmarkRole role : ordered_roles) {
			const VehicleSectionLandmark *landmark = findLandmark(role);
			if (landmark != nullptr) {
				profile.emplace_back(landmark->point().x, landmark->point().y);
			}
		}
		return profile;
	}

private:
	std::string identifier_;
	float station_z_ = 0.0f;
	std::vector<VehicleSectionLandmark> landmarks_;
};
