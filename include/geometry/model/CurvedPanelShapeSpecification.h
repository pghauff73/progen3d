#pragma once

#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>

class CurvedPanelShapeSpecification : public ShapeSpecification
{
public:
	CurvedPanelShapeSpecification(
		float width,
		float height,
		float horizontal_curvature,
		float vertical_curvature,
		float thickness,
		int horizontal_segments,
		int vertical_segments,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::CurvedPanel, std::move(key), detail_level),
		  width_(width),
		  height_(height),
		  horizontal_curvature_(horizontal_curvature),
		  vertical_curvature_(vertical_curvature),
		  thickness_(thickness),
		  horizontal_segments_(horizontal_segments),
		  vertical_segments_(vertical_segments),
		  canonical_text_(std::move(canonical_text))
	{
	}

	float width() const { return width_; }
	float height() const { return height_; }
	float horizontalCurvature() const { return horizontal_curvature_; }
	float verticalCurvature() const { return vertical_curvature_; }
	float thickness() const { return thickness_; }
	int horizontalSegments() const { return horizontal_segments_; }
	int verticalSegments() const { return vertical_segments_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return thickness_ > 0.0f; }

private:
	float width_ = 0.0f;
	float height_ = 0.0f;
	float horizontal_curvature_ = 0.0f;
	float vertical_curvature_ = 0.0f;
	float thickness_ = 0.0f;
	int horizontal_segments_ = 1;
	int vertical_segments_ = 1;
	std::string canonical_text_;
};
