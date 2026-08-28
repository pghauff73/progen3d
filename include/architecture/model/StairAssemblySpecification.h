#pragma once

#include <cstddef>
#include <string>
#include <utility>

class StairAssemblySpecification
{
public:
	StairAssemblySpecification(
		std::string object_identifier,
		float width,
		float total_rise,
		float total_run,
		std::size_t step_count,
		float tread_thickness,
		float nosing_projection,
		float stringer_width,
		float stringer_thickness,
		float balustrade_height,
		float glazing_thickness,
		std::string tread_material_identifier,
		std::string stringer_material_identifier,
		std::string glazing_material_identifier,
		std::string handrail_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  total_rise_(total_rise),
		  total_run_(total_run),
		  step_count_(step_count),
		  tread_thickness_(tread_thickness),
		  nosing_projection_(nosing_projection),
		  stringer_width_(stringer_width),
		  stringer_thickness_(stringer_thickness),
		  balustrade_height_(balustrade_height),
		  glazing_thickness_(glazing_thickness),
		  tread_material_identifier_(std::move(tread_material_identifier)),
		  stringer_material_identifier_(std::move(stringer_material_identifier)),
		  glazing_material_identifier_(std::move(glazing_material_identifier)),
		  handrail_material_identifier_(std::move(handrail_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float totalRise() const { return total_rise_; }
	float totalRun() const { return total_run_; }
	std::size_t stepCount() const { return step_count_; }
	float treadThickness() const { return tread_thickness_; }
	float nosingProjection() const { return nosing_projection_; }
	float stringerWidth() const { return stringer_width_; }
	float stringerThickness() const { return stringer_thickness_; }
	float balustradeHeight() const { return balustrade_height_; }
	float glazingThickness() const { return glazing_thickness_; }
	float risePerStep() const
	{
		return total_rise_ / static_cast<float>(step_count_);
	}
	float goingPerStep() const
	{
		return total_run_ / static_cast<float>(step_count_);
	}
	const std::string &treadMaterialIdentifier() const
	{
		return tread_material_identifier_;
	}
	const std::string &stringerMaterialIdentifier() const
	{
		return stringer_material_identifier_;
	}
	const std::string &glazingMaterialIdentifier() const
	{
		return glazing_material_identifier_;
	}
	const std::string &handrailMaterialIdentifier() const
	{
		return handrail_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float total_rise_ = 0.0f;
	float total_run_ = 0.0f;
	std::size_t step_count_ = 0u;
	float tread_thickness_ = 0.0f;
	float nosing_projection_ = 0.0f;
	float stringer_width_ = 0.0f;
	float stringer_thickness_ = 0.0f;
	float balustrade_height_ = 0.0f;
	float glazing_thickness_ = 0.0f;
	std::string tread_material_identifier_;
	std::string stringer_material_identifier_;
	std::string glazing_material_identifier_;
	std::string handrail_material_identifier_;
};
