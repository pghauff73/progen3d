#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class AxialProfilePolygon
{
public:
	AxialProfilePolygon(std::string name, std::vector<glm::vec2> vertices)
		: name_(std::move(name)), vertices_(std::move(vertices))
	{
	}

	const std::string &name() const
	{
		return name_;
	}

	const std::vector<glm::vec2> &vertices() const
	{
		return vertices_;
	}

private:
	std::string name_;
	std::vector<glm::vec2> vertices_;
};
