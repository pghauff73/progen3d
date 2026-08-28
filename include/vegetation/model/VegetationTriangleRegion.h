#pragma once

#include "vegetation/model/VegetationTriangleImportance.h"
#include "vegetation/model/VegetationTriangleRegionRole.h"

#include <cstddef>
#include <string>
#include <utility>

class VegetationTriangleRegion
{
public:
	VegetationTriangleRegion(
		std::string region_identifier,
		VegetationTriangleRegionRole role,
		std::size_t source_triangle_count,
		std::size_t minimum_triangle_count,
		double represented_surface_area,
		VegetationTriangleImportance importance)
		: region_identifier_(std::move(region_identifier)),
		  role_(role),
		  source_triangle_count_(source_triangle_count),
		  minimum_triangle_count_(minimum_triangle_count),
		  represented_surface_area_(represented_surface_area),
		  importance_(std::move(importance))
	{
	}

	const std::string &regionIdentifier() const { return region_identifier_; }
	VegetationTriangleRegionRole role() const { return role_; }
	std::size_t sourceTriangleCount() const { return source_triangle_count_; }
	std::size_t minimumTriangleCount() const { return minimum_triangle_count_; }
	double representedSurfaceArea() const { return represented_surface_area_; }
	const VegetationTriangleImportance &importance() const { return importance_; }

private:
	std::string region_identifier_;
	VegetationTriangleRegionRole role_ =
		VegetationTriangleRegionRole::FoliageInterior;
	std::size_t source_triangle_count_ = 0u;
	std::size_t minimum_triangle_count_ = 0u;
	double represented_surface_area_ = 0.0;
	VegetationTriangleImportance importance_{0.0, 0.0, 0.0, 0.0,
	                                        0.0, 0.0, 0.0, 0.0};
};
