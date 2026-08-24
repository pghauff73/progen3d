#pragma once

#include "geometry/model/SweepProfileShapeSpecification.h"

enum class EmbossedBeadSide
{
	Positive,
	Negative
};

enum class EmbossedBeadEndStyle
{
	Closed,
	Open
};

class EmbossedBeadShapeSpecification : public SweepProfileShapeSpecification
{
public:
	EmbossedBeadShapeSpecification(
		Profile2D profile,
		std::vector<glm::vec3> path_points,
		glm::vec3 up_hint,
		ExtrudeProfileCapPolicy cap_policy,
		float width,
		float depth,
		float shoulder_radius,
		EmbossedBeadSide side,
		EmbossedBeadEndStyle end_style,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: SweepProfileShapeSpecification(
			std::move(profile),
			std::move(path_points),
			up_hint,
			cap_policy,
			std::move(key),
			std::move(canonical_text),
			detail_level,
			ShapeFamily::EmbossedBead),
		  width_(width),
		  depth_(depth),
		  shoulder_radius_(shoulder_radius),
		  side_(side),
		  end_style_(end_style)
	{
	}

	float width() const { return width_; }
	float depth() const { return depth_; }
	float shoulderRadius() const { return shoulder_radius_; }
	EmbossedBeadSide side() const { return side_; }
	EmbossedBeadEndStyle endStyle() const { return end_style_; }

private:
	float width_ = 0.0f;
	float depth_ = 0.0f;
	float shoulder_radius_ = 0.0f;
	EmbossedBeadSide side_ = EmbossedBeadSide::Positive;
	EmbossedBeadEndStyle end_style_ = EmbossedBeadEndStyle::Closed;
};
