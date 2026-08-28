#pragma once

#include <string>
#include <utility>
#include <vector>

enum class StationInterpolationKind
{
	Pchip
};

class StationFunctionKnot
{
public:
	StationFunctionKnot(double position, double value)
		: position_(position), value_(value)
	{
	}

	double position() const { return position_; }
	double value() const { return value_; }

private:
	double position_ = 0.0;
	double value_ = 0.0;
};

class StationFunction
{
public:
	StationFunction(
		std::string identifier,
		StationInterpolationKind interpolation_kind,
		std::vector<StationFunctionKnot> knots)
		: identifier_(std::move(identifier)),
		  interpolation_kind_(interpolation_kind),
		  knots_(std::move(knots))
	{
	}

	const std::string &identifier() const { return identifier_; }
	StationInterpolationKind interpolationKind() const { return interpolation_kind_; }
	const std::vector<StationFunctionKnot> &knots() const { return knots_; }

private:
	std::string identifier_;
	StationInterpolationKind interpolation_kind_ = StationInterpolationKind::Pchip;
	std::vector<StationFunctionKnot> knots_;
};
