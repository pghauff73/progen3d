#pragma once

#include "geometry/model/Profile2D.h"

#include <cstddef>
#include <utility>

enum class ThinWallProfileFamily
{
	ClosedBox,
	OpenChannel,
	HatSection,
	MultiCellBox,
	Custom
};

class ThinWallProfile2D
{
public:
	ThinWallProfile2D(
		ThinWallProfileFamily family,
		Profile2D profile,
		float sheet_thickness,
		float bend_radius,
		std::size_t cell_count)
		: family_(family),
		  profile_(std::move(profile)),
		  sheet_thickness_(sheet_thickness),
		  bend_radius_(bend_radius),
		  cell_count_(cell_count)
	{
	}

	ThinWallProfileFamily family() const { return family_; }
	const Profile2D &profile() const { return profile_; }
	float sheetThickness() const { return sheet_thickness_; }
	float bendRadius() const { return bend_radius_; }
	std::size_t cellCount() const { return cell_count_; }

private:
	ThinWallProfileFamily family_ = ThinWallProfileFamily::Custom;
	Profile2D profile_{{{}, ProfileWindingCorrection::None}, {}};
	float sheet_thickness_ = 0.0f;
	float bend_radius_ = 0.0f;
	std::size_t cell_count_ = 0u;
};
