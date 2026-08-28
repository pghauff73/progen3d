#pragma once

#include <string>
#include <utility>

class HingeSpecification
{
public:
	HingeSpecification(
		std::string object_identifier,
		float cup_diameter,
		float cup_depth,
		float arm_length,
		float arm_width,
		float arm_depth,
		float plate_width,
		float plate_height,
		float plate_depth,
		std::string material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  cup_diameter_(cup_diameter),
		  cup_depth_(cup_depth),
		  arm_length_(arm_length),
		  arm_width_(arm_width),
		  arm_depth_(arm_depth),
		  plate_width_(plate_width),
		  plate_height_(plate_height),
		  plate_depth_(plate_depth),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float cupDiameter() const { return cup_diameter_; }
	float cupDepth() const { return cup_depth_; }
	float armLength() const { return arm_length_; }
	float armWidth() const { return arm_width_; }
	float armDepth() const { return arm_depth_; }
	float plateWidth() const { return plate_width_; }
	float plateHeight() const { return plate_height_; }
	float plateDepth() const { return plate_depth_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	std::string object_identifier_;
	float cup_diameter_ = 0.0f;
	float cup_depth_ = 0.0f;
	float arm_length_ = 0.0f;
	float arm_width_ = 0.0f;
	float arm_depth_ = 0.0f;
	float plate_width_ = 0.0f;
	float plate_height_ = 0.0f;
	float plate_depth_ = 0.0f;
	std::string material_identifier_;
};
