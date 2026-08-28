#pragma once

#include <optional>
#include <string>
#include <utility>

enum class InterfaceShape {
	Unspecified,
	Point,
	Circular,
	Rectangular,
	Axis
};

enum class InterfaceGender {
	Unspecified,
	Neutral,
	Male,
	Female
};

class InterfaceCompatibilityProfile {
public:
	InterfaceCompatibilityProfile(
		InterfaceShape shape = InterfaceShape::Unspecified,
		InterfaceGender gender = InterfaceGender::Unspecified,
		std::optional<float> nominal_diameter = std::nullopt,
		std::optional<float> nominal_width = std::nullopt,
		std::optional<float> nominal_height = std::nullopt,
		std::string connection_family = {})
		: shape_(shape),
		  gender_(gender),
		  nominal_diameter_(nominal_diameter),
		  nominal_width_(nominal_width),
		  nominal_height_(nominal_height),
		  connection_family_(std::move(connection_family)) {}

	InterfaceShape shape() const { return shape_; }
	InterfaceGender gender() const { return gender_; }
	const std::optional<float> &nominalDiameter() const { return nominal_diameter_; }
	const std::optional<float> &nominalWidth() const { return nominal_width_; }
	const std::optional<float> &nominalHeight() const { return nominal_height_; }
	const std::string &connectionFamily() const { return connection_family_; }

private:
	InterfaceShape shape_ = InterfaceShape::Unspecified;
	InterfaceGender gender_ = InterfaceGender::Unspecified;
	std::optional<float> nominal_diameter_;
	std::optional<float> nominal_width_;
	std::optional<float> nominal_height_;
	std::string connection_family_;
};
