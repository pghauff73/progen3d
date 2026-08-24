#pragma once

#include "geometry/model/AxialProfileAxis.h"
#include "geometry/model/AxialProfileCapPolicy.h"
#include "geometry/model/AxialProfileShapeSpecification.h"
#include "geometry/model/AxialTransitionKind.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct AxialProfilePolygonCandidate
{
	std::string name;
	std::vector<glm::vec2> vertices;
};

struct AxialProfileLevelCandidate
{
	float axial_position = 0.0f;
	std::string profile_name;
	glm::vec2 center{0.0f};
	glm::vec2 scale{1.0f};
	float rotation_degrees = 0.0f;
	AxialTransitionKind transition = AxialTransitionKind::Initial;
};

struct AxialProfileSpecificationCandidate
{
	AxialProfileAxis axis = AxialProfileAxis::Y;
	std::vector<AxialProfilePolygonCandidate> profiles;
	std::vector<AxialProfileLevelCandidate> levels;
	AxialProfileCapPolicy cap_policy = AxialProfileCapPolicy::createAll();
};

class AxialProfileSpecificationValidator
{
public:
	std::shared_ptr<const AxialProfileShapeSpecification> validate(
		AxialProfileSpecificationCandidate candidate,
		std::string *diagnostic) const;
};
