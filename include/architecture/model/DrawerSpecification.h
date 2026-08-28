#pragma once

#include <string>
#include <utility>

class DrawerSpecification
{
public:
	DrawerSpecification(
		std::string object_identifier,
		float width,
		float height,
		float depth,
		float panel_thickness,
		float front_thickness,
		float runner_width,
		float runner_height,
		std::string panel_material_identifier,
		std::string runner_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  depth_(depth),
		  panel_thickness_(panel_thickness),
		  front_thickness_(front_thickness),
		  runner_width_(runner_width),
		  runner_height_(runner_height),
		  panel_material_identifier_(std::move(panel_material_identifier)),
		  runner_material_identifier_(std::move(runner_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float panelThickness() const { return panel_thickness_; }
	float frontThickness() const { return front_thickness_; }
	float runnerWidth() const { return runner_width_; }
	float runnerHeight() const { return runner_height_; }
	const std::string &panelMaterialIdentifier() const
	{
		return panel_material_identifier_;
	}
	const std::string &runnerMaterialIdentifier() const
	{
		return runner_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float depth_ = 0.0f;
	float panel_thickness_ = 0.0f;
	float front_thickness_ = 0.0f;
	float runner_width_ = 0.0f;
	float runner_height_ = 0.0f;
	std::string panel_material_identifier_;
	std::string runner_material_identifier_;
};
