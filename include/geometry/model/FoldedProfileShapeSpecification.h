#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class FoldedProfileShapeSpecification : public ShapeSpecification
{
public:
	FoldedProfileShapeSpecification(
		std::vector<glm::vec2> fold_path,
		float thickness,
		float extrusion_depth,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			  ShapeFamily::FoldedProfile, std::move(key), detail_level),
		  fold_path_(std::move(fold_path)),
		  thickness_(thickness),
		  extrusion_depth_(extrusion_depth),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::vector<glm::vec2> &foldPath() const { return fold_path_; }
	float thickness() const { return thickness_; }
	float extrusionDepth() const { return extrusion_depth_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return cap_policy_.closesBothEnds(); }

private:
	std::vector<glm::vec2> fold_path_;
	float thickness_ = 0.0f;
	float extrusion_depth_ = 0.0f;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
