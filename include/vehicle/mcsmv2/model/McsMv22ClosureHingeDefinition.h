#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class McsMv22ClosureHingeDefinition
{
public:
	McsMv22ClosureHingeDefinition(
		std::string identifier,
		std::string closure_identifier,
		glm::dvec3 source_point,
		glm::dvec3 source_axis,
		double maximum_angle_degrees,
		double direction,
		double rise_metres,
		glm::dvec3 source_translation_axis,
		std::string joint_type)
		: identifier_(std::move(identifier)),
		  closure_identifier_(std::move(closure_identifier)),
		  source_point_(source_point),
		  source_axis_(source_axis),
		  maximum_angle_degrees_(maximum_angle_degrees),
		  direction_(direction),
		  rise_metres_(rise_metres),
		  source_translation_axis_(source_translation_axis),
		  joint_type_(std::move(joint_type))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &closureIdentifier() const { return closure_identifier_; }
	const glm::dvec3 &sourcePoint() const { return source_point_; }
	const glm::dvec3 &sourceAxis() const { return source_axis_; }
	double maximumAngleDegrees() const { return maximum_angle_degrees_; }
	double direction() const { return direction_; }
	double riseMetres() const { return rise_metres_; }
	const glm::dvec3 &sourceTranslationAxis() const
	{
		return source_translation_axis_;
	}
	const std::string &jointType() const { return joint_type_; }

private:
	std::string identifier_;
	std::string closure_identifier_;
	glm::dvec3 source_point_{0.0};
	glm::dvec3 source_axis_{0.0, 0.0, 1.0};
	double maximum_angle_degrees_ = 0.0;
	double direction_ = 1.0;
	double rise_metres_ = 0.0;
	glm::dvec3 source_translation_axis_{0.0, 0.0, 1.0};
	std::string joint_type_;
};
