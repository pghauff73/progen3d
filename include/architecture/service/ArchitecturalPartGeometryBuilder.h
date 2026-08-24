#pragma once

#include "architecture/model/ArchitecturalPartGeometry.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "geometry/model/RoundedBoxSpecification.h"

#include <string>

class ArchitecturalPartGeometryBuilder
{
public:
	explicit ArchitecturalPartGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalPartGeometry buildRectangularPrism(
		float width,
		float height,
		float depth,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

	ArchitecturalPartGeometry buildRoundedBox(
		const RoundedBoxSpecification &specification,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

	ArchitecturalPartGeometry buildCircularPrism(
		float diameter,
		float depth,
		int segments,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

	ArchitecturalPartGeometry buildShellLoftedCushion(
		float width,
		float height,
		float depth,
		float corner_radius,
		float shell_thickness,
		float middle_bulge,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

private:
	ArchitecturalPartGeometry buildExtrudedProfile(
		const Profile2D &profile,
		float depth,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

	GeometryComplexityLimits complexity_limits_;
};
