#pragma once

#include <glm/glm.hpp>

#include <vector>

enum class PolygonContainmentRelationship
{
	FirstContainsSecond,
	SecondContainsFirst,
	CrossingOrTouching,
	Disjoint
};

class PolygonContainmentAnalyzer
{
public:
	PolygonContainmentRelationship analyze(
		const std::vector<glm::vec2> &first,
		const std::vector<glm::vec2> &second) const;
};
