#pragma once

#include "vehicle/parametric/model/ModernCarCoordinateFrame.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

class ModernCarCoordinateConversionService
{
public:
	McpVehicleCoordinate convertSourceToMcp(
		const ModernCarCoordinateFrame &coordinate_frame,
		const ModernCarVariantDefinition &variant,
		const McsM1Coordinate &source) const
	{
		return coordinate_frame.convertSourceToMcp(
			source, variant.package().sourcePackageCenterStation());
	}

	McsM1Coordinate convertMcpToSource(
		const ModernCarCoordinateFrame &coordinate_frame,
		const ModernCarVariantDefinition &variant,
		const McpVehicleCoordinate &point) const
	{
		return coordinate_frame.convertMcpToSource(
			point, variant.package().sourcePackageCenterStation());
	}
};
