#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/Profile2D.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct Profile2DCandidate
{
	std::vector<glm::vec2> outer_loop;
	std::vector<std::vector<glm::vec2>> inner_loops;
};

class Profile2DValidator
{
public:
	explicit Profile2DValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const Profile2D> validate(
		Profile2DCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
