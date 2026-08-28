#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>

class PanelArraySpecification
{
public:
	PanelArraySpecification(
		std::string object_identifier,
		float panel_width,
		float panel_height,
		float panel_depth,
		std::size_t panel_count,
		glm::vec3 panel_spacing,
		std::string material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  panel_width_(panel_width),
		  panel_height_(panel_height),
		  panel_depth_(panel_depth),
		  panel_count_(panel_count),
		  panel_spacing_(panel_spacing),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float panelWidth() const { return panel_width_; }
	float panelHeight() const { return panel_height_; }
	float panelDepth() const { return panel_depth_; }
	std::size_t panelCount() const { return panel_count_; }
	const glm::vec3 &panelSpacing() const { return panel_spacing_; }
	const std::string &materialIdentifier() const { return material_identifier_; }

private:
	std::string object_identifier_;
	float panel_width_ = 0.0f;
	float panel_height_ = 0.0f;
	float panel_depth_ = 0.0f;
	std::size_t panel_count_ = 0u;
	glm::vec3 panel_spacing_{0.0f};
	std::string material_identifier_;
};
