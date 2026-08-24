#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class McsMv22HelicalGlassDefinition
{
public:
	McsMv22HelicalGlassDefinition(
		std::string identifier,
		std::string parent_closure_identifier,
		std::string side,
		double travel_metres,
		double inward_metres,
		double longitudinal_metres,
		double rotation_degrees,
		glm::dvec3 source_pivot)
		: identifier_(std::move(identifier)),
		  parent_closure_identifier_(std::move(parent_closure_identifier)),
		  side_(std::move(side)),
		  travel_metres_(travel_metres),
		  inward_metres_(inward_metres),
		  longitudinal_metres_(longitudinal_metres),
		  rotation_degrees_(rotation_degrees),
		  source_pivot_(source_pivot)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &parentClosureIdentifier() const
	{
		return parent_closure_identifier_;
	}
	const std::string &side() const { return side_; }
	double travelMetres() const { return travel_metres_; }
	double inwardMetres() const { return inward_metres_; }
	double longitudinalMetres() const { return longitudinal_metres_; }
	double rotationDegrees() const { return rotation_degrees_; }
	const glm::dvec3 &sourcePivot() const { return source_pivot_; }

private:
	std::string identifier_;
	std::string parent_closure_identifier_;
	std::string side_;
	double travel_metres_ = 0.0;
	double inward_metres_ = 0.0;
	double longitudinal_metres_ = 0.0;
	double rotation_degrees_ = 0.0;
	glm::dvec3 source_pivot_{0.0};
};
