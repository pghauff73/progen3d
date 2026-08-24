#pragma once

#include "geometry/model/Curve3D.h"
#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class SweepDiskShapeSpecification : public ShapeSpecification
{
public:
	SweepDiskShapeSpecification(
		Curve3D path_curve,
		std::vector<glm::vec3> sampled_path_points,
		glm::vec3 up_hint,
		float radius_start,
		float radius_end,
		int longitudinal_segments,
		int circumferential_segments,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::SweepDisk, std::move(key), detail_level),
		  path_curve_(std::move(path_curve)),
		  sampled_path_points_(std::move(sampled_path_points)),
		  up_hint_(up_hint),
		  radius_start_(radius_start),
		  radius_end_(radius_end),
		  longitudinal_segments_(longitudinal_segments),
		  circumferential_segments_(circumferential_segments),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const Curve3D &pathCurve() const { return path_curve_; }
	const std::vector<glm::vec3> &pathPoints() const { return sampled_path_points_; }
	const glm::vec3 &upHint() const { return up_hint_; }
	float radius() const { return radius_start_; }
	float radiusStart() const { return radius_start_; }
	float radiusEnd() const { return radius_end_; }
	bool hasConstantRadius() const { return radius_start_ == radius_end_; }
	int longitudinalSegments() const { return longitudinal_segments_; }
	int circumferentialSegments() const { return circumferential_segments_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return cap_policy_.closesBothEnds(); }

private:
	Curve3D path_curve_{Curve3DType::Line, {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}};
	std::vector<glm::vec3> sampled_path_points_;
	glm::vec3 up_hint_{0.0f, 0.0f, 1.0f};
	float radius_start_ = 0.0f;
	float radius_end_ = 0.0f;
	int longitudinal_segments_ = 1;
	int circumferential_segments_ = 16;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
