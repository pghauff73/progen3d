#pragma once

#include "geometry/model/Curve3DEvaluator.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CurveArcLengthTable;

enum class Curve3DType
{
	Line,
	Polyline,
	Bezier,
	CatmullRom
};

class Curve3D : public Curve3DEvaluator
{
public:
	Curve3D(
		Curve3DType type,
		std::vector<glm::vec3> control_points,
		std::string identifier = {})
		: type_(type),
		  control_points_(std::move(control_points)),
		  identifier_(std::move(identifier))
	{
	}

	Curve3DType type() const { return type_; }
	const std::vector<glm::vec3> &controlPoints() const { return control_points_; }
	const std::string &identifier() const { return identifier_; }

	bool isValid(std::string *diagnostic = nullptr) const override;
	glm::dvec3 evaluatePosition(double parameter) const override;
	glm::dvec3 evaluateFirstDerivative(double parameter) const override;
	glm::dvec3 evaluateSecondDerivative(double parameter) const override;
	glm::vec3 evaluate(float parameter) const;
	glm::vec3 evaluateDerivative(float parameter) const;
	glm::vec3 evaluateSecondDerivative(float parameter) const;
	std::vector<glm::vec3> sample(std::size_t sample_count) const;
	double length(std::size_t arc_length_sample_count = 257u) const;
	double parameterAtDistance(
		double distance,
		std::size_t arc_length_sample_count = 257u) const;
	glm::dvec3 evaluateByArcFraction(
		double arc_fraction,
		std::size_t arc_length_sample_count = 257u) const;
	std::vector<glm::dvec3> sampleByArcLength(
		std::size_t sample_count,
		std::size_t arc_length_sample_count = 257u) const;

private:
	std::shared_ptr<const CurveArcLengthTable> arcLengthTable(
		std::size_t sample_count) const;

	Curve3DType type_ = Curve3DType::Line;
	std::vector<glm::vec3> control_points_;
	std::string identifier_;
	mutable std::size_t cached_arc_length_sample_count_ = 0u;
	mutable std::shared_ptr<const CurveArcLengthTable> cached_arc_length_table_;
};
