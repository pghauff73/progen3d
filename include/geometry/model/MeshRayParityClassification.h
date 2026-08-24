#pragma once

#include <cstddef>

enum class MeshPointContainment
{
	Outside,
	Inside,
	OnSurface,
	Indeterminate
};

class MeshRayParityClassification
{
public:
	MeshRayParityClassification(
		MeshPointContainment containment,
		std::size_t unique_intersection_count,
		bool complete)
		: containment_(containment),
		  unique_intersection_count_(unique_intersection_count),
		  complete_(complete)
	{
	}

	MeshPointContainment containment() const { return containment_; }
	std::size_t uniqueIntersectionCount() const
	{
		return unique_intersection_count_;
	}
	bool complete() const { return complete_; }

private:
	MeshPointContainment containment_ = MeshPointContainment::Indeterminate;
	std::size_t unique_intersection_count_ = 0u;
	bool complete_ = false;
};
