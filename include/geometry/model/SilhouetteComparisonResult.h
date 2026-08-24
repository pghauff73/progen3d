#pragma once

#include <cstddef>

class SilhouetteComparisonResult
{
public:
	SilhouetteComparisonResult(
		std::size_t intersection_pixels,
		std::size_t union_pixels,
		double intersection_over_union)
		: intersection_pixels_(intersection_pixels), union_pixels_(union_pixels),
		  intersection_over_union_(intersection_over_union)
	{
	}
	std::size_t intersectionPixels() const { return intersection_pixels_; }
	std::size_t unionPixels() const { return union_pixels_; }
	double intersectionOverUnion() const { return intersection_over_union_; }
private:
	std::size_t intersection_pixels_ = 0u;
	std::size_t union_pixels_ = 0u;
	double intersection_over_union_ = 0.0;
};
