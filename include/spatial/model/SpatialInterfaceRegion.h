#pragma once

#include <string>
#include <utility>

enum class SpatialInterfaceRegionKind {
	Point,
	PlaneRectangle,
	AxisSegment,
	ObjectBoundaryFace
};

class SpatialInterfaceRegion {
public:
	static SpatialInterfaceRegion point()
	{
		return SpatialInterfaceRegion(SpatialInterfaceRegionKind::Point, 0.0f, 0.0f, {});
	}

	static SpatialInterfaceRegion planeRectangle(float width, float height)
	{
		return SpatialInterfaceRegion(
			SpatialInterfaceRegionKind::PlaneRectangle, width, height, {});
	}

	static SpatialInterfaceRegion axisSegment(float length)
	{
		return SpatialInterfaceRegion(
			SpatialInterfaceRegionKind::AxisSegment, length, 0.0f, {});
	}

	static SpatialInterfaceRegion objectBoundaryFace(std::string face_name)
	{
		return SpatialInterfaceRegion(
			SpatialInterfaceRegionKind::ObjectBoundaryFace,
			0.0f,
			0.0f,
			std::move(face_name));
	}

	SpatialInterfaceRegionKind kind() const { return kind_; }
	float primaryExtent() const { return primary_extent_; }
	float secondaryExtent() const { return secondary_extent_; }
	const std::string &boundaryFaceName() const { return boundary_face_name_; }

private:
	SpatialInterfaceRegion(SpatialInterfaceRegionKind kind,
	                       float primary_extent,
	                       float secondary_extent,
	                       std::string boundary_face_name)
		: kind_(kind),
		  primary_extent_(primary_extent),
		  secondary_extent_(secondary_extent),
		  boundary_face_name_(std::move(boundary_face_name)) {}

	SpatialInterfaceRegionKind kind_ = SpatialInterfaceRegionKind::Point;
	float primary_extent_ = 0.0f;
	float secondary_extent_ = 0.0f;
	std::string boundary_face_name_;
};
