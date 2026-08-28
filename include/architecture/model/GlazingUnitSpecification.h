#pragma once

#include <string>
#include <utility>

class GlazingUnitSpecification
{
public:
	GlazingUnitSpecification(
		std::string object_identifier,
		float width,
		float height,
		float glass_thickness,
		float air_gap,
		float spacer_width,
		std::string glass_material_identifier,
		std::string spacer_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  glass_thickness_(glass_thickness),
		  air_gap_(air_gap),
		  spacer_width_(spacer_width),
		  glass_material_identifier_(std::move(glass_material_identifier)),
		  spacer_material_identifier_(std::move(spacer_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float glassThickness() const { return glass_thickness_; }
	float airGap() const { return air_gap_; }
	float spacerWidth() const { return spacer_width_; }
	float totalDepth() const { return glass_thickness_ * 2.0f + air_gap_; }
	const std::string &glassMaterialIdentifier() const
	{
		return glass_material_identifier_;
	}
	const std::string &spacerMaterialIdentifier() const
	{
		return spacer_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float glass_thickness_ = 0.0f;
	float air_gap_ = 0.0f;
	float spacer_width_ = 0.0f;
	std::string glass_material_identifier_;
	std::string spacer_material_identifier_;
};
