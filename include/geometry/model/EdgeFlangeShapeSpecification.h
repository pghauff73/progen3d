#pragma once

#include "geometry/model/SweepProfileShapeSpecification.h"

enum class EdgeFlangeSide
{
	Positive,
	Negative
};

class EdgeFlangeShapeSpecification : public SweepProfileShapeSpecification
{
public:
	EdgeFlangeShapeSpecification(
		Profile2D profile,
		std::vector<glm::vec3> boundary_path,
		glm::vec3 up_hint,
		float width,
		float thickness,
		float angle_degrees,
		float bend_radius,
		EdgeFlangeSide side,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: SweepProfileShapeSpecification(
			std::move(profile),
			std::move(boundary_path),
			up_hint,
			ExtrudeProfileCapPolicy::createAll(),
			std::move(key),
			std::move(canonical_text),
			detail_level,
			ShapeFamily::EdgeFlange),
		  width_(width),
		  thickness_(thickness),
		  angle_degrees_(angle_degrees),
		  bend_radius_(bend_radius),
		  side_(side)
	{
	}

	float width() const { return width_; }
	float thickness() const { return thickness_; }
	float angleDegrees() const { return angle_degrees_; }
	float bendRadius() const { return bend_radius_; }
	EdgeFlangeSide side() const { return side_; }

private:
	float width_ = 0.0f;
	float thickness_ = 0.0f;
	float angle_degrees_ = 0.0f;
	float bend_radius_ = 0.0f;
	EdgeFlangeSide side_ = EdgeFlangeSide::Positive;
};
