#pragma once

#include <cstddef>
#include <string>
#include <utility>

enum class ParametricGenerationResolution
{
	Low,
	Medium,
	High
};

class ParametricModelGenerationPolicy
{
public:
	ParametricModelGenerationPolicy(
		std::string identifier,
		ParametricGenerationResolution resolution,
		std::size_t longitudinal_samples,
		std::size_t lateral_samples,
		std::size_t vertical_samples,
		double iso_value,
		bool allow_station_extrapolation)
		: identifier_(std::move(identifier)),
		  resolution_(resolution),
		  longitudinal_samples_(longitudinal_samples),
		  lateral_samples_(lateral_samples),
		  vertical_samples_(vertical_samples),
		  iso_value_(iso_value),
		  allow_station_extrapolation_(allow_station_extrapolation)
	{
	}

	const std::string &identifier() const { return identifier_; }
	ParametricGenerationResolution resolution() const { return resolution_; }
	std::size_t longitudinalSamples() const { return longitudinal_samples_; }
	std::size_t lateralSamples() const { return lateral_samples_; }
	std::size_t verticalSamples() const { return vertical_samples_; }
	double isoValue() const { return iso_value_; }
	bool allowsStationExtrapolation() const { return allow_station_extrapolation_; }

private:
	std::string identifier_;
	ParametricGenerationResolution resolution_ = ParametricGenerationResolution::Low;
	std::size_t longitudinal_samples_ = 0u;
	std::size_t lateral_samples_ = 0u;
	std::size_t vertical_samples_ = 0u;
	double iso_value_ = 0.0;
	bool allow_station_extrapolation_ = false;
};
