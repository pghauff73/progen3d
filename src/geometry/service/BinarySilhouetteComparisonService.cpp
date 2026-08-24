#include "geometry/service/BinarySilhouetteComparisonService.h"

SilhouetteComparisonResult BinarySilhouetteComparisonService::compare(
	const BinarySilhouette &candidate,
	const BinarySilhouette &reference,
	std::string *diagnostic) const
{
	if (candidate.width() != reference.width() ||
	    candidate.height() != reference.height()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Silhouette comparison requires identical raster dimensions.";
		}
		return SilhouetteComparisonResult(0u, 0u, 0.0);
	}
	std::size_t intersection = 0u;
	std::size_t union_count = 0u;
	for (std::size_t y = 0u; y < candidate.height(); ++y) {
		for (std::size_t x = 0u; x < candidate.width(); ++x) {
			const bool candidate_pixel = candidate.isOccupied(x, y);
			const bool reference_pixel = reference.isOccupied(x, y);
			if (candidate_pixel && reference_pixel) ++intersection;
			if (candidate_pixel || reference_pixel) ++union_count;
		}
	}
	const double iou = union_count == 0u
		? 1.0 : static_cast<double>(intersection) / static_cast<double>(union_count);
	return SilhouetteComparisonResult(intersection, union_count, iou);
}
