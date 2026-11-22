#pragma once

#include "geometry/model/FacialExpressionControls.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

// An open, continuous front-face surface with three anatomical apertures.
// This first milestone is an analytical deformation rig, not a tissue solver.
class FacialSkinShapeSpecification : public ShapeSpecification
{
public:
	FacialSkinShapeSpecification(glm::vec3 size,
	                            int horizontal_segments,
	                            int vertical_segments,
	                            FacialExpressionControls controls,
	                            ShapeSpecificationKey key,
	                            std::string canonical_text)
		: ShapeSpecification(ShapeFamily::FacialSkin, std::move(key)),
		  size_(size),
		  horizontal_segments_(horizontal_segments),
		  vertical_segments_(vertical_segments),
		  controls_(controls),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const glm::vec3 &size() const { return size_; }
	int horizontalSegments() const { return horizontal_segments_; }
	int verticalSegments() const { return vertical_segments_; }
	const FacialExpressionControls &controls() const { return controls_; }
	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	glm::vec3 size_{0.16f, 0.21f, 0.045f};
	int horizontal_segments_ = 48;
	int vertical_segments_ = 64;
	FacialExpressionControls controls_;
	std::string canonical_text_;
};
