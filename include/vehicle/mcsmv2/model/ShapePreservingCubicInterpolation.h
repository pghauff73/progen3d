#pragma once

#include <utility>
#include <vector>

class ShapePreservingCubicInterpolation
{
public:
	ShapePreservingCubicInterpolation(
		std::vector<double> coordinates,
		std::vector<double> values,
		std::vector<double> derivatives)
		: coordinates_(std::move(coordinates)),
		  values_(std::move(values)),
		  derivatives_(std::move(derivatives))
	{
	}

	double evaluateAt(double coordinate) const;
	const std::vector<double> &coordinates() const { return coordinates_; }
	const std::vector<double> &values() const { return values_; }
	const std::vector<double> &derivatives() const { return derivatives_; }

private:
	std::vector<double> coordinates_;
	std::vector<double> values_;
	std::vector<double> derivatives_;
};
