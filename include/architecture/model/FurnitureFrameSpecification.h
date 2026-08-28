#pragma once

#include <string>
#include <utility>

class FurnitureFrameSpecification
{
public:
	FurnitureFrameSpecification(
		std::string object_identifier,
		float width,
		float height,
		float depth,
		float member_thickness,
		std::string material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  depth_(depth),
		  member_thickness_(member_thickness),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float memberThickness() const { return member_thickness_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float depth_ = 0.0f;
	float member_thickness_ = 0.0f;
	std::string material_identifier_;
};
