#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

class SimplePolygonTriangulator
{
public:
	bool triangulate(const std::vector<glm::vec2> &counter_clockwise_vertices,
	                 std::vector<glm::ivec3> *triangles,
	                 std::string *diagnostic) const;
};
