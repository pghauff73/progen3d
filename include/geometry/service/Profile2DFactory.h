#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/Profile2D.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class Profile2DFactory
{
public:
	explicit Profile2DFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const Profile2D> createRectangle(
		float width, float height, std::string *diagnostic) const;
	std::shared_ptr<const Profile2D> createRoundedRectangle(
		float width,
		float height,
		float corner_radius,
		int segments_per_corner,
		std::string *diagnostic) const;
	std::shared_ptr<const Profile2D> createCircle(
		float radius, int segments, std::string *diagnostic) const;
	std::shared_ptr<const Profile2D> createEllipse(
		float radius_x,
		float radius_y,
		int segments,
		std::string *diagnostic) const;
	std::shared_ptr<const Profile2D> createChamferRectangle(
		float width,
		float height,
		float chamfer,
		std::string *diagnostic) const;
	std::shared_ptr<const Profile2D> createPolygon(
		std::vector<glm::vec2> outer_loop,
		std::vector<std::vector<glm::vec2>> inner_loops,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
