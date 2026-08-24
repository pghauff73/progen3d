#pragma once

#include <cstddef>
#include <limits>

class MeshSurfaceDistanceReport
{
public:
	MeshSurfaceDistanceReport() = default;

	MeshSurfaceDistanceReport(
		double minimum_distance,
		std::size_t evaluated_triangle_pair_count,
		bool complete,
		bool finite)
		: minimum_distance_(minimum_distance),
		  evaluated_triangle_pair_count_(evaluated_triangle_pair_count),
		  complete_(complete),
		  finite_(finite)
	{
	}

	double minimumDistance() const { return minimum_distance_; }
	std::size_t evaluatedTrianglePairCount() const
	{
		return evaluated_triangle_pair_count_;
	}
	bool complete() const { return complete_; }
	bool finite() const { return finite_; }

private:
	double minimum_distance_ = std::numeric_limits<double>::infinity();
	std::size_t evaluated_triangle_pair_count_ = 0u;
	bool complete_ = false;
	bool finite_ = false;
};
