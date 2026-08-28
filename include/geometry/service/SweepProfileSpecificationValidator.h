#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/PanelCutShapeSpecification.h"
#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/service/Profile2DValidator.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct SweepProfileShapeSpecificationCandidate
{
	Profile2DCandidate profile;
	std::vector<glm::vec3> path_points;
	glm::vec3 up_hint{0.0f, 0.0f, 1.0f};
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class SweepProfileSpecificationValidator
{
public:
	explicit SweepProfileSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const SweepProfileShapeSpecification> validate(
		SweepProfileShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;
	std::shared_ptr<const PanelCutShapeSpecification> validatePanelCut(
		SweepProfileShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	std::shared_ptr<const SweepProfileShapeSpecification> validateAsFamily(
		SweepProfileShapeSpecificationCandidate candidate,
		ShapeFamily family,
		std::string *diagnostic) const;
	GeometryComplexityLimits complexity_limits_;
};
