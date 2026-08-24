#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/Profile2D.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class SweepFramePolicy
{
	RotationMinimizing,
	ReferenceAligned
};

class VariableSectionSweepStationSpecification
{
public:
	VariableSectionSweepStationSpecification(
		float normalized_path_position,
		Profile2D profile,
		glm::vec2 center = glm::vec2(0.0f),
		glm::vec2 scale = glm::vec2(1.0f),
		float rotation_degrees = 0.0f)
		: normalized_path_position_(normalized_path_position),
		  profile_(std::move(profile)),
		  center_(center),
		  scale_(scale),
		  rotation_degrees_(rotation_degrees)
	{
	}

	float normalizedPathPosition() const { return normalized_path_position_; }
	const Profile2D &profile() const { return profile_; }
	const glm::vec2 &center() const { return center_; }
	const glm::vec2 &scale() const { return scale_; }
	float rotationDegrees() const { return rotation_degrees_; }

private:
	float normalized_path_position_ = 0.0f;
	Profile2D profile_{{{}, ProfileWindingCorrection::None}, {}};
	glm::vec2 center_{0.0f};
	glm::vec2 scale_{1.0f};
	float rotation_degrees_ = 0.0f;
};

class VariableSectionSweepShapeSpecification : public ShapeSpecification
{
public:
	VariableSectionSweepShapeSpecification(
		std::vector<glm::vec3> path_points,
		std::vector<VariableSectionSweepStationSpecification> stations,
		std::vector<Profile2D> nominal_section_profiles,
		glm::vec3 up_hint,
		SweepFramePolicy frame_policy,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			  ShapeFamily::VariableSectionSweep,
			  std::move(key),
			  detail_level),
		  path_points_(std::move(path_points)),
		  stations_(std::move(stations)),
		  nominal_section_profiles_(std::move(nominal_section_profiles)),
		  up_hint_(up_hint),
		  frame_policy_(frame_policy),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::vector<glm::vec3> &pathPoints() const { return path_points_; }
	const std::vector<VariableSectionSweepStationSpecification> &stations() const
	{
		return stations_;
	}
	const std::vector<Profile2D> &nominalSectionProfiles() const
	{
		return nominal_section_profiles_;
	}
	const glm::vec3 &upHint() const { return up_hint_; }
	SweepFramePolicy framePolicy() const { return frame_policy_; }
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override
	{
		return cap_policy_.closesBothEnds();
	}

private:
	std::vector<glm::vec3> path_points_;
	std::vector<VariableSectionSweepStationSpecification> stations_;
	std::vector<Profile2D> nominal_section_profiles_;
	glm::vec3 up_hint_{0.0f, 1.0f, 0.0f};
	SweepFramePolicy frame_policy_ = SweepFramePolicy::RotationMinimizing;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
