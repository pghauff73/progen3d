#pragma once

#include "geometry/model/GeometryDetailLevel.h"

#include <cstddef>
#include <string>
#include <utility>

class ImplicitSurfaceGenerationRequest
{
public:
	ImplicitSurfaceGenerationRequest(
		std::string identifier,
		std::size_t first_axis_samples,
		std::size_t second_axis_samples,
		std::size_t third_axis_samples,
		double iso_value,
		int smoothing_iterations,
		double smoothing_expansion_factor,
		double smoothing_contraction_factor,
		GeometryDetailLevel detail_level)
		: identifier_(std::move(identifier)),
		  first_axis_samples_(first_axis_samples),
		  second_axis_samples_(second_axis_samples),
		  third_axis_samples_(third_axis_samples),
		  iso_value_(iso_value),
		  smoothing_iterations_(smoothing_iterations),
		  smoothing_expansion_factor_(smoothing_expansion_factor),
		  smoothing_contraction_factor_(smoothing_contraction_factor),
		  detail_level_(detail_level)
	{
	}

	const std::string &identifier() const { return identifier_; }
	std::size_t firstAxisSamples() const { return first_axis_samples_; }
	std::size_t secondAxisSamples() const { return second_axis_samples_; }
	std::size_t thirdAxisSamples() const { return third_axis_samples_; }
	double isoValue() const { return iso_value_; }
	int smoothingIterations() const { return smoothing_iterations_; }
	double smoothingExpansionFactor() const
	{
		return smoothing_expansion_factor_;
	}
	double smoothingContractionFactor() const
	{
		return smoothing_contraction_factor_;
	}
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::string identifier_;
	std::size_t first_axis_samples_ = 0u;
	std::size_t second_axis_samples_ = 0u;
	std::size_t third_axis_samples_ = 0u;
	double iso_value_ = 0.0;
	int smoothing_iterations_ = 0;
	double smoothing_expansion_factor_ = 0.0;
	double smoothing_contraction_factor_ = 0.0;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::CoarseShape;
};
