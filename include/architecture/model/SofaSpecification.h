#pragma once

#include <string>
#include <utility>

class SofaSpecification
{
public:
	SofaSpecification(
		std::string object_identifier,
		float width,
		float height,
		float depth,
		float base_height,
		float arm_width,
		float leg_height,
		float cushion_gap,
		std::string frame_material_identifier,
		std::string upholstery_material_identifier,
		std::string leg_material_identifier)
		: object_identifier_(std::move(object_identifier)),
		  width_(width),
		  height_(height),
		  depth_(depth),
		  base_height_(base_height),
		  arm_width_(arm_width),
		  leg_height_(leg_height),
		  cushion_gap_(cushion_gap),
		  frame_material_identifier_(std::move(frame_material_identifier)),
		  upholstery_material_identifier_(
			  std::move(upholstery_material_identifier)),
		  leg_material_identifier_(std::move(leg_material_identifier))
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float depth() const { return depth_; }
	float baseHeight() const { return base_height_; }
	float armWidth() const { return arm_width_; }
	float legHeight() const { return leg_height_; }
	float cushionGap() const { return cushion_gap_; }
	const std::string &frameMaterialIdentifier() const
	{
		return frame_material_identifier_;
	}
	const std::string &upholsteryMaterialIdentifier() const
	{
		return upholstery_material_identifier_;
	}
	const std::string &legMaterialIdentifier() const
	{
		return leg_material_identifier_;
	}

private:
	std::string object_identifier_;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float depth_ = 0.0f;
	float base_height_ = 0.0f;
	float arm_width_ = 0.0f;
	float leg_height_ = 0.0f;
	float cushion_gap_ = 0.0f;
	std::string frame_material_identifier_;
	std::string upholstery_material_identifier_;
	std::string leg_material_identifier_;
};
