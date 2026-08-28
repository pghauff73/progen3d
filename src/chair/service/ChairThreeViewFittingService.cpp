#include "chair/service/ChairThreeViewFittingService.h"

#include "geometry/service/BinarySilhouetteComparisonService.h"

ChairThreeViewFitReport ChairThreeViewFittingService::compare(
	const std::string &chair_identifier,
	const ThreeViewProjection &candidate,
	const ThreeViewProjection &reference,
	int matched_iteration,
	std::string *diagnostic) const
{
	std::vector<ChairViewFitRecord> records;
	BinarySilhouetteComparisonService comparison_service;
	for (OrthographicProjectionView view : {
			 OrthographicProjectionView::Front,
			 OrthographicProjectionView::Side,
			 OrthographicProjectionView::Top}) {
		const SilhouetteComparisonResult result = comparison_service.compare(
			candidate.silhouette(view), reference.silhouette(view), diagnostic);
		records.emplace_back(view, result.intersectionOverUnion(), matched_iteration);
	}
	return ChairThreeViewFitReport(chair_identifier, std::move(records));
}
