#pragma once

#include "geometry/model/Profile2D.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct ProfileTriangle2D
{
	glm::vec2 first{0.0f};
	glm::vec2 second{0.0f};
	glm::vec2 third{0.0f};
};

class ProfileCapTriangulator
{
public:
	bool triangulate(const Profile2D &profile,
	                 std::vector<ProfileTriangle2D> *triangles,
	                 std::string *diagnostic) const;
};
