#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class WindowOpeningSpecification
{
public:
	WindowOpeningSpecification(
		std::string object_identifier,
		float wall_width,
		float wall_height,
		float wall_depth,
		glm::vec2 opening_center,
		float opening_width,
		float opening_height)
		: object_identifier_(std::move(object_identifier)),
		  wall_width_(wall_width),
		  wall_height_(wall_height),
		  wall_depth_(wall_depth),
		  opening_center_(opening_center),
		  opening_width_(opening_width),
		  opening_height_(opening_height)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float wallWidth() const { return wall_width_; }
	float wallHeight() const { return wall_height_; }
	float wallDepth() const { return wall_depth_; }
	const glm::vec2 &openingCenter() const { return opening_center_; }
	float openingWidth() const { return opening_width_; }
	float openingHeight() const { return opening_height_; }

private:
	std::string object_identifier_;
	float wall_width_ = 0.0f;
	float wall_height_ = 0.0f;
	float wall_depth_ = 0.0f;
	glm::vec2 opening_center_{0.0f};
	float opening_width_ = 0.0f;
	float opening_height_ = 0.0f;
};
