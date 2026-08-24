#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/Profile2D.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>

class ExtrudeProfileShapeSpecification : public ShapeSpecification
{
public:
	ExtrudeProfileShapeSpecification(
		Profile2D profile,
		float depth,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals,
		ShapeFamily family = ShapeFamily::ExtrudeProfile)
		: ShapeSpecification(
			  family, std::move(key), detail_level),
		  profile_(std::move(profile)),
		  depth_(depth),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const Profile2D &profile() const { return profile_; }
	float depth() const { return depth_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return cap_policy_.closesBothEnds(); }

private:
	Profile2D profile_;
	float depth_ = 0.0f;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
