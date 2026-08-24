#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/Profile2D.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>

class RevolveShapeSpecification : public ShapeSpecification
{
public:
	RevolveShapeSpecification(
		Profile2D radial_profile,
		float start_degrees,
		float sweep_degrees,
		int angular_segments,
		ExtrudeProfileCapPolicy angular_cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(ShapeFamily::Revolve, std::move(key), detail_level),
		  radial_profile_(std::move(radial_profile)),
		  start_degrees_(start_degrees),
		  sweep_degrees_(sweep_degrees),
		  angular_segments_(angular_segments),
		  angular_cap_policy_(angular_cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const Profile2D &radialProfile() const { return radial_profile_; }
	float startDegrees() const { return start_degrees_; }
	float sweepDegrees() const { return sweep_degrees_; }
	int angularSegments() const { return angular_segments_; }
	const ExtrudeProfileCapPolicy &angularCapPolicy() const
	{
		return angular_cap_policy_;
	}
	bool isFullRevolution() const { return sweep_degrees_ == 360.0f; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return isFullRevolution() || angular_cap_policy_.closesBothEnds();
	}

private:
	Profile2D radial_profile_{{{}, ProfileWindingCorrection::None}, {}};
	float start_degrees_ = 0.0f;
	float sweep_degrees_ = 360.0f;
	int angular_segments_ = 32;
	ExtrudeProfileCapPolicy angular_cap_policy_ =
		ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
