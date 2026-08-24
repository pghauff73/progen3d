#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/Profile2D.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class SweepProfileShapeSpecification : public ShapeSpecification
{
public:
	SweepProfileShapeSpecification(
		Profile2D profile,
		std::vector<glm::vec3> path_points,
		glm::vec3 up_hint,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals,
		ShapeFamily family = ShapeFamily::SweepProfile)
		: ShapeSpecification(
			  family, std::move(key), detail_level),
		  profile_(std::move(profile)),
		  path_points_(std::move(path_points)),
		  up_hint_(up_hint),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const Profile2D &profile() const { return profile_; }
	const std::vector<glm::vec3> &pathPoints() const { return path_points_; }
	const glm::vec3 &upHint() const { return up_hint_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return cap_policy_.closesBothEnds(); }

private:
	Profile2D profile_{{{}, ProfileWindingCorrection::None}, {}};
	std::vector<glm::vec3> path_points_;
	glm::vec3 up_hint_{0.0f, 0.0f, 1.0f};
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
