#pragma once

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class VehicleLoftPanelSection
{
public:
	VehicleLoftPanelSection(float station_z, std::vector<glm::vec2> profile)
		: station_z_(station_z), profile_(std::move(profile))
	{
	}

	float stationZ() const { return station_z_; }
	const std::vector<glm::vec2> &profile() const { return profile_; }

private:
	float station_z_ = 0.0f;
	std::vector<glm::vec2> profile_;
};
