#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

class SimplePolygonValidator
{
public:
	bool normalize(const std::vector<glm::vec2> &input_vertices,
	               std::vector<glm::vec2> *normalized_vertices,
	               std::string *diagnostic) const;

	static float signedArea(const std::vector<glm::vec2> &vertices);
};
