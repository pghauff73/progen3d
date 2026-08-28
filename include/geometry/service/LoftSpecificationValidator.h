#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/LoftShapeSpecification.h"
#include "geometry/model/ShellLoftShapeSpecification.h"
#include "geometry/model/SurfaceLoftShapeSpecification.h"
#include "geometry/service/Profile2DValidator.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct LoftSectionCandidate
{
	float axial_position = 0.0f;
	Profile2DCandidate profile;
	glm::vec2 center{0.0f};
	glm::vec2 scale{1.0f};
	float rotation_degrees = 0.0f;
};

struct LoftShapeSpecificationCandidate
{
	std::vector<LoftSectionCandidate> sections;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

struct ShellLoftSectionCandidate
{
	float axial_position = 0.0f;
	std::vector<glm::vec2> outer_loop;
	std::vector<glm::vec2> inner_loop;
	glm::vec2 center{0.0f};
	glm::vec2 scale{1.0f};
	float rotation_degrees = 0.0f;
};

struct ShellLoftShapeSpecificationCandidate
{
	std::vector<ShellLoftSectionCandidate> sections;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class LoftSpecificationValidator
{
public:
	explicit LoftSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const LoftShapeSpecification> validateLoft(
		LoftShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;
	std::shared_ptr<const SurfaceLoftShapeSpecification> validateSurfaceLoft(
		LoftShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;
	std::shared_ptr<const ShellLoftShapeSpecification> validateShellLoft(
		ShellLoftShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
