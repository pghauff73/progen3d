#include "vegetation/service/VegetationRepresentationFidelityEvaluationService.h"

#include <array>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace {

bool valid_percent(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 100.0;
}

std::size_t view_index(VegetationReferenceView view)
{
	switch (view) {
	case VegetationReferenceView::Front: return 0u;
	case VegetationReferenceView::Right: return 1u;
	case VegetationReferenceView::Top: return 2u;
	}
	return 3u;
}

void compare_metric(
	std::vector<VegetationRepresentationFidelityIssue> *issues,
	VegetationReferenceView view,
	const std::string &metric_name,
	double required_percent,
	double observed_percent)
{
	if (observed_percent < required_percent) {
		issues->emplace_back(
			view, metric_name, required_percent, observed_percent);
	}
}

} // namespace

VegetationRepresentationFidelityReport
VegetationRepresentationFidelityEvaluationService::evaluate(
	const VegetationRepresentationFidelityProfile &profile,
	const std::vector<VegetationViewFidelityObservation> &observations) const
{
	const double thresholds[] = {
		profile.minimumSilhouetteMatchPercent(),
		profile.minimumProjectedFoliageAreaMatchPercent(),
		profile.minimumCrownGapFractionMatchPercent(),
		profile.minimumBranchTopologyMatchPercent(),
		profile.minimumMaterialAreaMatchPercent(),
		profile.minimumMotionEnvelopeMatchPercent()};
	for (double threshold : thresholds) {
		if (!valid_percent(threshold)) {
			return VegetationRepresentationFidelityReport(
				{}, "Vegetation fidelity thresholds must be finite percentages.");
		}
	}
	if (observations.size() != 3u) {
		return VegetationRepresentationFidelityReport(
			{}, "Vegetation fidelity evaluation requires front, right, and top observations.");
	}

	std::array<bool, 3> observed_views{false, false, false};
	std::vector<VegetationRepresentationFidelityIssue> issues;
	for (const VegetationViewFidelityObservation &observation : observations) {
		const std::size_t index = view_index(observation.view());
		if (index >= observed_views.size() || observed_views[index]) {
			return VegetationRepresentationFidelityReport(
				{}, "Vegetation fidelity observations contain duplicate or unknown views.");
		}
		observed_views[index] = true;
		const double values[] = {
			observation.silhouetteMatchPercent(),
			observation.projectedFoliageAreaMatchPercent(),
			observation.crownGapFractionMatchPercent(),
			observation.branchTopologyMatchPercent(),
			observation.materialAreaMatchPercent(),
			observation.motionEnvelopeMatchPercent()};
		for (double value : values) {
			if (!valid_percent(value)) {
				return VegetationRepresentationFidelityReport(
					{}, "Vegetation fidelity observations must be finite percentages.");
			}
		}
		compare_metric(
			&issues, observation.view(), "Silhouette",
			profile.minimumSilhouetteMatchPercent(),
			observation.silhouetteMatchPercent());
		compare_metric(
			&issues, observation.view(), "ProjectedFoliageArea",
			profile.minimumProjectedFoliageAreaMatchPercent(),
			observation.projectedFoliageAreaMatchPercent());
		compare_metric(
			&issues, observation.view(), "CrownGapFraction",
			profile.minimumCrownGapFractionMatchPercent(),
			observation.crownGapFractionMatchPercent());
		compare_metric(
			&issues, observation.view(), "BranchTopology",
			profile.minimumBranchTopologyMatchPercent(),
			observation.branchTopologyMatchPercent());
		compare_metric(
			&issues, observation.view(), "MaterialArea",
			profile.minimumMaterialAreaMatchPercent(),
			observation.materialAreaMatchPercent());
		compare_metric(
			&issues, observation.view(), "MotionEnvelope",
			profile.minimumMotionEnvelopeMatchPercent(),
			observation.motionEnvelopeMatchPercent());
	}
	return VegetationRepresentationFidelityReport(std::move(issues));
}
