#pragma once

#include <string>
#include <utility>
#include <vector>

class ParametricCharacterCurve
{
public:
	explicit ParametricCharacterCurve(std::string identifier)
		: identifier_(std::move(identifier))
	{
	}

	const std::string &identifier() const { return identifier_; }

private:
	std::string identifier_;
};

class ParametricCharacterCurveNetwork
{
public:
	ParametricCharacterCurveNetwork(
		std::vector<ParametricCharacterCurve> curves,
		std::size_t samples_per_curve)
		: curves_(std::move(curves)), samples_per_curve_(samples_per_curve)
	{
	}

	const std::vector<ParametricCharacterCurve> &curves() const { return curves_; }
	std::size_t samplesPerCurve() const { return samples_per_curve_; }

private:
	std::vector<ParametricCharacterCurve> curves_;
	std::size_t samples_per_curve_ = 0u;
};
