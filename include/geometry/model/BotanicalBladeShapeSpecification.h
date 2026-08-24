#pragma once

#include "geometry/model/BotanicalBladeProfile.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>

class BotanicalBladeShapeSpecification : public ShapeSpecification
{
public:
	BotanicalBladeProfile profile() const { return profile_; }
	float length() const { return length_; }
	float maximumWidth() const { return maximum_width_; }
	float longitudinalCurvature() const { return longitudinal_curvature_; }
	float camber() const { return camber_; }
	float twistDegrees() const { return twist_degrees_; }
	float thickness() const { return thickness_; }
	float widthPower() const { return width_power_; }
	int longitudinalSegments() const { return longitudinal_segments_; }
	int lateralSegments() const { return lateral_segments_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return thickness_ > 0.0f; }

protected:
	BotanicalBladeShapeSpecification(
		ShapeFamily family,
		BotanicalBladeProfile profile,
		float length,
		float maximum_width,
		float longitudinal_curvature,
		float camber,
		float twist_degrees,
		float thickness,
		float width_power,
		int longitudinal_segments,
		int lateral_segments,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: ShapeSpecification(family, std::move(key), detail_level),
		  profile_(profile),
		  length_(length),
		  maximum_width_(maximum_width),
		  longitudinal_curvature_(longitudinal_curvature),
		  camber_(camber),
		  twist_degrees_(twist_degrees),
		  thickness_(thickness),
		  width_power_(width_power),
		  longitudinal_segments_(longitudinal_segments),
		  lateral_segments_(lateral_segments),
		  canonical_text_(std::move(canonical_text))
	{
	}

private:
	BotanicalBladeProfile profile_ = BotanicalBladeProfile::Elliptic;
	float length_ = 0.1f;
	float maximum_width_ = 0.05f;
	float longitudinal_curvature_ = 0.0f;
	float camber_ = 0.0f;
	float twist_degrees_ = 0.0f;
	float thickness_ = 0.0f;
	float width_power_ = 1.0f;
	int longitudinal_segments_ = 12;
	int lateral_segments_ = 4;
	std::string canonical_text_;
};

class LeafBladeShapeSpecification final : public BotanicalBladeShapeSpecification
{
public:
	LeafBladeShapeSpecification(
		BotanicalBladeProfile profile,
		float length,
		float maximum_width,
		float longitudinal_curvature,
		float camber,
		float twist_degrees,
		float thickness,
		float width_power,
		int longitudinal_segments,
		int lateral_segments,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: BotanicalBladeShapeSpecification(
			  ShapeFamily::LeafBlade, profile, length, maximum_width,
			  longitudinal_curvature, camber, twist_degrees, thickness,
			  width_power, longitudinal_segments, lateral_segments,
			  std::move(key), std::move(canonical_text), detail_level)
	{
	}
};

class PetalBladeShapeSpecification final : public BotanicalBladeShapeSpecification
{
public:
	PetalBladeShapeSpecification(
		BotanicalBladeProfile profile,
		float length,
		float maximum_width,
		float longitudinal_curvature,
		float camber,
		float twist_degrees,
		float thickness,
		float width_power,
		int longitudinal_segments,
		int lateral_segments,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: BotanicalBladeShapeSpecification(
			  ShapeFamily::PetalBlade, profile, length, maximum_width,
			  longitudinal_curvature, camber, twist_degrees, thickness,
			  width_power, longitudinal_segments, lateral_segments,
			  std::move(key), std::move(canonical_text), detail_level)
	{
	}
};

