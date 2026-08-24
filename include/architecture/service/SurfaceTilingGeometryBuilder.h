#pragma once

#include "architecture/model/SurfaceTilingGeometry.h"
#include "architecture/model/SurfaceTilingSpecification.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <optional>
#include <string>

struct SurfaceTilingBuildResult
{
	std::optional<SurfaceTilingGeometry> geometry;
	GeometryBuildStatus status = GeometryBuildStatus::UnsupportedTopology;
	std::string diagnostic;

	bool succeeded() const { return geometry.has_value(); }
};

class SurfaceTilingGeometryBuilder
{
public:
	explicit SurfaceTilingGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	SurfaceTilingBuildResult build(
		const SurfaceTilingSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
