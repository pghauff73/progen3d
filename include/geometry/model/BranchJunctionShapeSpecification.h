#pragma once

#include "geometry/model/BranchJunctionArmSpecification.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>
#include <vector>

class BranchJunctionShapeSpecification : public ShapeSpecification
{
public:
	BranchJunctionShapeSpecification(
		float core_radius,
		float bulge_scale,
		std::vector<BranchJunctionArmSpecification> arms,
		int circumferential_segments,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			  ShapeFamily::BranchJunction, std::move(key), detail_level),
		  core_radius_(core_radius),
		  bulge_scale_(bulge_scale),
		  arms_(std::move(arms)),
		  circumferential_segments_(circumferential_segments),
		  canonical_text_(std::move(canonical_text))
	{
	}

	float coreRadius() const { return core_radius_; }
	float bulgeScale() const { return bulge_scale_; }
	const std::vector<BranchJunctionArmSpecification> &arms() const
	{
		return arms_;
	}
	int circumferentialSegments() const { return circumferential_segments_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	float core_radius_ = 0.0f;
	float bulge_scale_ = 1.0f;
	std::vector<BranchJunctionArmSpecification> arms_;
	int circumferential_segments_ = 12;
	std::string canonical_text_;
};
