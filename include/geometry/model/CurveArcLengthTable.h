#pragma once

#include <utility>
#include <vector>

class CurveArcLengthSample
{
public:
	CurveArcLengthSample(double parameter, double distance)
		: parameter_(parameter), distance_(distance)
	{
	}

	double parameter() const { return parameter_; }
	double distance() const { return distance_; }

private:
	double parameter_ = 0.0;
	double distance_ = 0.0;
};

class CurveArcLengthTable
{
public:
	explicit CurveArcLengthTable(std::vector<CurveArcLengthSample> samples)
		: samples_(std::move(samples))
	{
	}

	const std::vector<CurveArcLengthSample> &samples() const { return samples_; }
	double totalLength() const
	{
		return samples_.empty() ? 0.0 : samples_.back().distance();
	}
	double parameterAtDistance(double distance) const;

private:
	std::vector<CurveArcLengthSample> samples_;
};
