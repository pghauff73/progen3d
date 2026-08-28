#pragma once

#include <cstddef>

class MeshIntersectionScreeningReport
{
public:
	MeshIntersectionScreeningReport() = default;

	MeshIntersectionScreeningReport(
		std::size_t candidate_triangle_pair_count,
		std::size_t checked_triangle_pair_count,
		std::size_t intersection_count,
		bool complete)
		: candidate_triangle_pair_count_(candidate_triangle_pair_count),
		  checked_triangle_pair_count_(checked_triangle_pair_count),
		  intersection_count_(intersection_count),
		  complete_(complete)
	{
	}

	std::size_t candidateTrianglePairCount() const
	{
		return candidate_triangle_pair_count_;
	}
	std::size_t checkedTrianglePairCount() const
	{
		return checked_triangle_pair_count_;
	}
	std::size_t intersectionCount() const { return intersection_count_; }
	bool complete() const { return complete_; }
	bool passed() const { return complete_ && intersection_count_ == 0u; }

private:
	std::size_t candidate_triangle_pair_count_ = 0u;
	std::size_t checked_triangle_pair_count_ = 0u;
	std::size_t intersection_count_ = 0u;
	bool complete_ = false;
};
