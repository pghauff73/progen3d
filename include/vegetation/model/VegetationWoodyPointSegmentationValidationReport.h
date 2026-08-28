#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyPointSegmentationValidationReport
{
public:
	VegetationWoodyPointSegmentationValidationReport(
		std::size_t woody_point_count,
		std::size_t assigned_woody_point_count,
		std::size_t unassigned_woody_point_count,
		double minimum_assignment_confidence,
		std::vector<std::string> rejection_reasons)
		: woody_point_count_(woody_point_count),
		  assigned_woody_point_count_(assigned_woody_point_count),
		  unassigned_woody_point_count_(unassigned_woody_point_count),
		  minimum_assignment_confidence_(minimum_assignment_confidence),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool acceptedForGraphConstruction() const
	{
		return rejection_reasons_.empty();
	}
	std::size_t woodyPointCount() const { return woody_point_count_; }
	std::size_t assignedWoodyPointCount() const
	{
		return assigned_woody_point_count_;
	}
	std::size_t unassignedWoodyPointCount() const
	{
		return unassigned_woody_point_count_;
	}
	double minimumAssignmentConfidence() const
	{
		return minimum_assignment_confidence_;
	}
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::size_t woody_point_count_ = 0u;
	std::size_t assigned_woody_point_count_ = 0u;
	std::size_t unassigned_woody_point_count_ = 0u;
	double minimum_assignment_confidence_ = 0.0;
	std::vector<std::string> rejection_reasons_;
};
