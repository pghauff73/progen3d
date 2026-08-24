#pragma once

#include <string>
#include <utility>

class WindowFrameSpecification
{
public:
	WindowFrameSpecification(
		std::string object_identifier,
		float width,
		float height,
		float perimeter_clearance,
		int mullion_count,
		float glazing_thickness)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  perimeter_clearance_(perimeter_clearance),
		  mullion_count_(mullion_count),
		  glazing_thickness_(glazing_thickness)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float perimeterClearance() const { return perimeter_clearance_; }
	int mullionCount() const { return mullion_count_; }
	float glazingThickness() const { return glazing_thickness_; }

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float perimeter_clearance_ = 0.0f;
	int mullion_count_ = 0;
	float glazing_thickness_ = 0.0f;
};
