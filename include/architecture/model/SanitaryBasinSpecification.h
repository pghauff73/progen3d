#pragma once

#include <string>
#include <utility>

class SanitaryBasinSpecification
{
public:
	SanitaryBasinSpecification(
		std::string object_identifier,
		float width,
		float depth,
		float height,
		float wall_thickness,
		float drain_diameter,
		float drain_length,
		std::string basin_material_identifier,
		std::string drain_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  depth_(depth),
		  height_(height),
		  wall_thickness_(wall_thickness),
		  drain_diameter_(drain_diameter),
		  drain_length_(drain_length),
		  basin_material_identifier_(std::move(basin_material_identifier)),
		  drain_material_identifier_(std::move(drain_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float depth() const { return depth_; }
	float height() const { return height_; }
	float wallThickness() const { return wall_thickness_; }
	float drainDiameter() const { return drain_diameter_; }
	float drainLength() const { return drain_length_; }
	const std::string &basinMaterialIdentifier() const
	{
		return basin_material_identifier_;
	}
	const std::string &drainMaterialIdentifier() const
	{
		return drain_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float depth_ = 0.0f;
	float height_ = 0.0f;
	float wall_thickness_ = 0.0f;
	float drain_diameter_ = 0.0f;
	float drain_length_ = 0.0f;
	std::string basin_material_identifier_;
	std::string drain_material_identifier_;
};
