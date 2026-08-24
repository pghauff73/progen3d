#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/ThinWallProfile2D.h"

#include <memory>
#include <string>

class ThinWallProfileFactory
{
public:
	explicit ThinWallProfileFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const ThinWallProfile2D> createClosedBox(
		float width,
		float height,
		float sheet_thickness,
		float bend_radius,
		std::string *diagnostic) const;

	std::shared_ptr<const ThinWallProfile2D> createOpenChannel(
		float width,
		float height,
		float sheet_thickness,
		float bend_radius,
		std::string *diagnostic) const;

	std::shared_ptr<const ThinWallProfile2D> createHatSection(
		float width,
		float height,
		float sheet_thickness,
		float bend_radius,
		float flange_width,
		std::string *diagnostic) const;

	std::shared_ptr<const ThinWallProfile2D> createMultiCellBox(
		float width,
		float height,
		float sheet_thickness,
		float bend_radius,
		std::size_t cell_count,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
