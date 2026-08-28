#pragma once

#include <string>
#include <utility>

class PanelBoxSpecification
{
public:
	PanelBoxSpecification(
		std::string object_identifier,
		float width,
		float height,
		float depth,
		float panel_thickness,
		float back_panel_thickness,
		bool include_front_panel,
		std::string material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  depth_(depth),
		  panel_thickness_(panel_thickness),
		  back_panel_thickness_(back_panel_thickness),
		  include_front_panel_(include_front_panel),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float panelThickness() const { return panel_thickness_; }
	float backPanelThickness() const { return back_panel_thickness_; }
	bool includesFrontPanel() const { return include_front_panel_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float depth_ = 0.0f;
	float panel_thickness_ = 0.0f;
	float back_panel_thickness_ = 0.0f;
	bool include_front_panel_ = false;
	std::string material_identifier_;
};
