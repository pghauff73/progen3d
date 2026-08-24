#pragma once

#include <string>
#include <utility>

class SurfaceTilingSpecification
{
public:
	SurfaceTilingSpecification(
		float surface_width,
		float surface_height,
		float tile_width,
		float tile_height,
		float joint_width,
		float tile_depth,
		std::string material_identifier)
		: surface_width_(surface_width),
		  surface_height_(surface_height),
		  tile_width_(tile_width),
		  tile_height_(tile_height),
		  joint_width_(joint_width),
		  tile_depth_(tile_depth),
		  material_identifier_(std::move(material_identifier))
	{
	}

	float surfaceWidth() const { return surface_width_; }
	float surfaceHeight() const { return surface_height_; }
	float tileWidth() const { return tile_width_; }
	float tileHeight() const { return tile_height_; }
	float jointWidth() const { return joint_width_; }
	float tileDepth() const { return tile_depth_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	float surface_width_ = 0.0f;
	float surface_height_ = 0.0f;
	float tile_width_ = 0.0f;
	float tile_height_ = 0.0f;
	float joint_width_ = 0.0f;
	float tile_depth_ = 0.0f;
	std::string material_identifier_;
};
