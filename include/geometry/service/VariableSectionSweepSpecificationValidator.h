#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/VariableSectionSweepShapeSpecification.h"
#include "geometry/service/Profile2DValidator.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct VariableSectionSweepStationCandidate
{
	float normalized_path_position = 0.0f;
	Profile2DCandidate profile;
	glm::vec2 center{0.0f};
	glm::vec2 scale{1.0f};
	float rotation_degrees = 0.0f;
};

struct VariableSectionSweepShapeSpecificationCandidate
{
	std::vector<glm::vec3> path_points;
	std::vector<VariableSectionSweepStationCandidate> stations;
	std::vector<Profile2DCandidate> nominal_section_profiles;
	glm::vec3 up_hint{0.0f, 1.0f, 0.0f};
	SweepFramePolicy frame_policy = SweepFramePolicy::RotationMinimizing;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class VariableSectionSweepSpecificationValidator
{
public:
	explicit VariableSectionSweepSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const VariableSectionSweepShapeSpecification> validate(
		VariableSectionSweepShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
