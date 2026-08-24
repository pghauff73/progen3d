#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class TaperedSweepShapeSpecification : public ShapeSpecification
{
public:
	TaperedSweepShapeSpecification(
		std::vector<glm::vec3> path_points,
		std::vector<float> radii,
		glm::vec3 up_hint,
		int circumferential_segments,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			  ShapeFamily::TaperedSweep, std::move(key), detail_level),
		  path_points_(std::move(path_points)),
		  radii_(std::move(radii)),
		  up_hint_(up_hint),
		  circumferential_segments_(circumferential_segments),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::vector<glm::vec3> &pathPoints() const { return path_points_; }
	const std::vector<float> &radii() const { return radii_; }
	const glm::vec3 &upHint() const { return up_hint_; }
	int circumferentialSegments() const { return circumferential_segments_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return cap_policy_.closesBothEnds();
	}

private:
	std::vector<glm::vec3> path_points_;
	std::vector<float> radii_;
	glm::vec3 up_hint_{0.0f, 0.0f, 1.0f};
	int circumferential_segments_ = 12;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};

