#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/SweepDiskShapeSpecification.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct SweepDiskShapeSpecificationCandidate
{
	std::vector<glm::vec3> path_points;
	Curve3DType curve_type = Curve3DType::Polyline;
	std::vector<glm::vec3> curve_control_points;
	bool curve_was_explicit = false;
	glm::vec3 up_hint{0.0f, 0.0f, 1.0f};
	float radius = 0.05f;
	bool radius_was_explicit = false;
	float radius_start = 0.0f;
	float radius_end = 0.0f;
	bool radius_start_was_explicit = false;
	bool radius_end_was_explicit = false;
	int longitudinal_segments = 0;
	int circumferential_segments = 16;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class SweepDiskSpecificationValidator
{
public:
	explicit SweepDiskSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const SweepDiskShapeSpecification> validate(
		SweepDiskShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
