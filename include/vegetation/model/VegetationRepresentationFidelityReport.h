#pragma once

#include "vegetation/model/VegetationReferenceView.h"

#include <string>
#include <utility>
#include <vector>

class VegetationRepresentationFidelityIssue
{
public:
	VegetationRepresentationFidelityIssue(
		VegetationReferenceView view,
		std::string metric_name,
		double required_percent,
		double observed_percent)
		: view_(view),
		  metric_name_(std::move(metric_name)),
		  required_percent_(required_percent),
		  observed_percent_(observed_percent)
	{
	}

	VegetationReferenceView view() const { return view_; }
	const std::string &metricName() const { return metric_name_; }
	double requiredPercent() const { return required_percent_; }
	double observedPercent() const { return observed_percent_; }

private:
	VegetationReferenceView view_ = VegetationReferenceView::Front;
	std::string metric_name_;
	double required_percent_ = 0.0;
	double observed_percent_ = 0.0;
};

class VegetationRepresentationFidelityReport
{
public:
	VegetationRepresentationFidelityReport(
		std::vector<VegetationRepresentationFidelityIssue> issues,
		std::string diagnostic = {})
		: issues_(std::move(issues)), diagnostic_(std::move(diagnostic))
	{
	}

	bool passed() const { return issues_.empty() && diagnostic_.empty(); }
	const std::vector<VegetationRepresentationFidelityIssue> &issues() const
	{
		return issues_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	std::vector<VegetationRepresentationFidelityIssue> issues_;
	std::string diagnostic_;
};
