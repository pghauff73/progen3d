#include "vegetation/service/VegetationWoodyAxisQualityEvaluationService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

double dot(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	return first.x() * second.x() + first.y() * second.y() +
	       first.z() * second.z();
}

VegetationMeasuredPoint3d subtract(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	return VegetationMeasuredPoint3d(
		first.x() - second.x(), first.y() - second.y(),
		first.z() - second.z());
}

double axial_projection(
	const VegetationMeasuredPoint3d &point,
	const VegetationMeasuredPoint3d &centroid,
	const VegetationMeasuredPoint3d &axis)
{
	return dot(subtract(point, centroid), axis);
}

double radial_distance(
	const VegetationMeasuredPoint3d &point,
	const VegetationMeasuredPoint3d &centroid,
	const VegetationMeasuredPoint3d &axis)
{
	const VegetationMeasuredPoint3d offset = subtract(point, centroid);
	const double projection = dot(offset, axis);
	const double radial_x = offset.x() - axis.x() * projection;
	const double radial_y = offset.y() - axis.y() * projection;
	const double radial_z = offset.z() - axis.z() * projection;
	return std::sqrt(
		radial_x * radial_x + radial_y * radial_y + radial_z * radial_z);
}

double interpolated_radius(
	double projection,
	const std::vector<VegetationWoodyAxisRadiusStation> &stations)
{
	if (stations.empty()) return 0.0;
	if (projection <= stations.front().axialProjectionMetres()) {
		return stations.front().radiusMetres();
	}
	if (projection >= stations.back().axialProjectionMetres()) {
		return stations.back().radiusMetres();
	}
	for (std::size_t index = 1u; index < stations.size(); ++index) {
		const auto &upper = stations[index];
		if (projection > upper.axialProjectionMetres()) continue;
		const auto &lower = stations[index - 1u];
		const double span = upper.axialProjectionMetres() -
		                    lower.axialProjectionMetres();
		if (span <= 0.0) return lower.radiusMetres();
		const double factor = std::clamp(
			(projection - lower.axialProjectionMetres()) / span, 0.0, 1.0);
		return lower.radiusMetres() +
		       (upper.radiusMetres() - lower.radiusMetres()) * factor;
	}
	return stations.back().radiusMetres();
}

double surface_rmse(
	const std::vector<VegetationMeasuredPoint3d> &points,
	const VegetationMeasuredPoint3d &centroid,
	const VegetationMeasuredPoint3d &axis,
	const std::vector<VegetationWoodyAxisRadiusStation> &stations)
{
	if (points.empty() || stations.empty()) {
		return std::numeric_limits<double>::infinity();
	}
	double squared_error_sum = 0.0;
	for (const auto &point : points) {
		const double projection = axial_projection(point, centroid, axis);
		const double residual = radial_distance(point, centroid, axis) -
		                        interpolated_radius(projection, stations);
		squared_error_sum += residual * residual;
	}
	return std::sqrt(squared_error_sum / static_cast<double>(points.size()));
}

}

VegetationWoodyAxisQualityReport
VegetationWoodyAxisQualityEvaluationService::evaluate(
	const VegetationWoodyAxisQualityEvidence &evidence,
	const VegetationWoodyAxisReconstructionPolicy &policy) const
{
	const double woody_label_fraction = evidence.totalPointCount() == 0u
		                                    ? 0.0
		                                    : static_cast<double>(
			                                      evidence.woodyPointCount()) /
			                                      evidence.totalPointCount();
	const double excluded_outlier_fraction =
		evidence.initialTrainingWoodyPointCount() == 0u
			? 0.0
			: static_cast<double>(evidence.excludedTrainingOutlierCount()) /
			      evidence.initialTrainingWoodyPointCount();
	const double variance_sum = evidence.minorVariance() +
	                            evidence.intermediateVariance() +
	                            evidence.principalVariance();
	const double principal_variance_fraction = variance_sum > 0.0
		                                           ? evidence.principalVariance() /
			                                             variance_sum
		                                           : 0.0;
	const double training_surface_rmse = surface_rmse(
		evidence.trainingPointsMetres(), evidence.axisCentroidMetres(),
		evidence.principalAxisDirection(), evidence.radiusStations());
	const double holdout_surface_rmse = surface_rmse(
		evidence.holdoutPointsMetres(), evidence.axisCentroidMetres(),
		evidence.principalAxisDirection(), evidence.radiusStations());
	std::size_t covered_holdout_point_count = 0u;
	if (!evidence.radiusStations().empty()) {
		const double minimum_projection =
			evidence.radiusStations().front().axialProjectionMetres();
		const double maximum_projection =
			evidence.radiusStations().back().axialProjectionMetres();
		for (const auto &point : evidence.holdoutPointsMetres()) {
			const double projection = axial_projection(
				point, evidence.axisCentroidMetres(),
				evidence.principalAxisDirection());
			if (projection >= minimum_projection &&
			    projection <= maximum_projection) {
				++covered_holdout_point_count;
			}
		}
	}
	const double holdout_axial_coverage = evidence.holdoutPointsMetres().empty()
		                                        ? 0.0
		                                        : static_cast<double>(
			                                          covered_holdout_point_count) /
			                                          evidence.holdoutPointsMetres()
			                                              .size();
	std::size_t taper_violation_count = 0u;
	for (std::size_t index = 1u; index < evidence.radiusStations().size();
	     ++index) {
		if (evidence.radiusStations()[index].radiusMetres() >
		    evidence.radiusStations()[index - 1u].radiusMetres() + 1e-9) {
			++taper_violation_count;
		}
	}
	const std::size_t taper_comparison_count =
		evidence.radiusStations().empty()
			? 0u
			: evidence.radiusStations().size() - 1u;
	const double taper_violation_fraction = taper_comparison_count == 0u
		                                         ? 0.0
		                                         : static_cast<double>(
			                                           taper_violation_count) /
			                                           taper_comparison_count;

	std::vector<std::string> rejection_reasons;
	if (evidence.trainingPointsMetres().empty()) {
		rejection_reasons.emplace_back(
			"Primary woody-axis training evidence is empty.");
	}
	if (evidence.holdoutPointsMetres().empty()) {
		rejection_reasons.emplace_back(
			"Primary woody-axis holdout evidence is empty.");
	}
	if (evidence.radiusStations().size() < 2u || evidence.cylinders().empty()) {
		rejection_reasons.emplace_back(
			"Primary woody-axis geometry has insufficient axial support.");
	}
	if (woody_label_fraction < policy.minimumWoodyLabelFraction()) {
		rejection_reasons.emplace_back(
			"Woody label fraction is below the reconstruction boundary.");
	}
	if (excluded_outlier_fraction >
	    policy.maximumExcludedOutlierFraction()) {
		rejection_reasons.emplace_back(
			"Excluded training outlier fraction exceeds the reconstruction boundary.");
	}
	if (principal_variance_fraction <
	    policy.minimumPrincipalVarianceFraction()) {
		rejection_reasons.emplace_back(
			"Principal-axis variance fraction is below the reconstruction boundary.");
	}
	if (training_surface_rmse >
	    policy.maximumTrainingSurfaceRmseMetres()) {
		rejection_reasons.emplace_back(
			"Training cylinder-surface RMSE exceeds the reconstruction boundary.");
	}
	if (holdout_surface_rmse > policy.maximumHoldoutSurfaceRmseMetres()) {
		rejection_reasons.emplace_back(
			"Holdout cylinder-surface RMSE exceeds the reconstruction boundary.");
	}
	if (holdout_axial_coverage <
	    policy.minimumHoldoutAxialCoverageFraction()) {
		rejection_reasons.emplace_back(
			"Holdout axial coverage is below the reconstruction boundary.");
	}
	if (taper_violation_fraction >
	    policy.maximumTaperViolationFraction()) {
		rejection_reasons.emplace_back(
			"Primary woody-axis taper violations exceed the reconstruction boundary.");
	}
	return VegetationWoodyAxisQualityReport(
		evidence.reconstructionIdentifier(), evidence.datasetIdentifier(),
		evidence.sourcePayloadSha256(), evidence.trainingPointsMetres().size(),
		evidence.holdoutPointsMetres().size(),
		evidence.excludedTrainingOutlierCount(), evidence.radiusStations().size(),
		evidence.cylinders().size(), woody_label_fraction,
		excluded_outlier_fraction, principal_variance_fraction,
		training_surface_rmse, holdout_surface_rmse, holdout_axial_coverage,
		taper_violation_fraction, std::move(rejection_reasons));
}
