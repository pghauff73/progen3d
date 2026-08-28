#include "vegetation/service/VegetationPrimaryWoodyAxisReconstructionService.h"

#include "vegetation/service/VegetationWoodyAxisQualityEvaluationService.h"

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationWoodyAxisReconstructionIssue;
using IssueCode = VegetationWoodyAxisReconstructionIssueCode;

constexpr const char *reconstruction_schema =
	"ProGen3D-VegetationPrimaryWoodyAxisReconstruction-v1";
constexpr const char *job_schema =
	"ProGen3D-VegetationPointCloudReconstructionJob-v1";
constexpr const char *algorithm_identifier =
	"ProGen3D-PrimaryWoodyAxis-v1";

struct IndexedWoodyPoint
{
	std::size_t source_index = 0u;
	VegetationMeasuredPoint3d position_metres{0.0, 0.0, 0.0};
};

struct PrincipalAxisSolution
{
	VegetationMeasuredPoint3d centroid_metres{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d direction{0.0, 0.0, 1.0};
	double minor_variance = 0.0;
	double intermediate_variance = 0.0;
	double principal_variance = 0.0;
};

struct AxialStationAccumulator
{
	std::vector<double> projections_metres;
	std::vector<double> radial_distances_metres;
};

VegetationWoodyAxisReconstructionReport report(
	std::vector<Issue> issues,
	std::optional<VegetationWoodyAxisQualityReport> quality_report,
	std::optional<VegetationWoodyAxisReconstruction> reconstruction)
{
	return VegetationWoodyAxisReconstructionReport(
		std::move(issues), std::move(quality_report),
		std::move(reconstruction));
}

bool fraction(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool valid_policy(const VegetationWoodyAxisReconstructionPolicy &policy)
{
	return std::isfinite(policy.axialStationSpacingMetres()) &&
	       policy.axialStationSpacingMetres() > 0.0 &&
	       policy.minimumWoodyPointCount() > 0u &&
	       policy.minimumTrainingWoodyPointCount() > 2u &&
	       policy.minimumPointsPerAxialStation() > 0u &&
	       policy.maximumCylinderCount() > 0u &&
	       policy.holdoutDivisor() > 1u &&
	       std::isfinite(policy.radialOutlierMadMultiplier()) &&
	       policy.radialOutlierMadMultiplier() > 0.0 &&
	       fraction(policy.maximumExcludedOutlierFraction()) &&
	       fraction(policy.minimumWoodyLabelFraction()) &&
	       fraction(policy.minimumPrincipalVarianceFraction()) &&
	       std::isfinite(policy.maximumTrainingSurfaceRmseMetres()) &&
	       policy.maximumTrainingSurfaceRmseMetres() >= 0.0 &&
	       std::isfinite(policy.maximumHoldoutSurfaceRmseMetres()) &&
	       policy.maximumHoldoutSurfaceRmseMetres() >= 0.0 &&
	       fraction(policy.minimumHoldoutAxialCoverageFraction()) &&
	       fraction(policy.maximumTaperViolationFraction()) &&
	       std::isfinite(policy.minimumObservedRadiusMetres()) &&
	       policy.minimumObservedRadiusMetres() > 0.0;
}

Eigen::Vector3d vector(const VegetationMeasuredPoint3d &point)
{
	return Eigen::Vector3d(point.x(), point.y(), point.z());
}

VegetationMeasuredPoint3d point(const Eigen::Vector3d &vector_value)
{
	return VegetationMeasuredPoint3d(
		vector_value.x(), vector_value.y(), vector_value.z());
}

std::optional<PrincipalAxisSolution> solve_principal_axis(
	const std::vector<VegetationMeasuredPoint3d> &points)
{
	if (points.size() < 3u) return std::nullopt;
	Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
	for (const auto &sample : points) centroid += vector(sample);
	centroid /= static_cast<double>(points.size());
	Eigen::Matrix3d covariance = Eigen::Matrix3d::Zero();
	for (const auto &sample : points) {
		const Eigen::Vector3d offset = vector(sample) - centroid;
		covariance += offset * offset.transpose();
	}
	covariance /= static_cast<double>(points.size());
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(covariance);
	if (solver.info() != Eigen::Success) return std::nullopt;
	const Eigen::Vector3d eigenvalues = solver.eigenvalues();
	if (!eigenvalues.allFinite() || eigenvalues.z() <= 0.0) {
		return std::nullopt;
	}
	Eigen::Vector3d direction = solver.eigenvectors().col(2).normalized();
	Eigen::Index dominant_component = 0;
	direction.cwiseAbs().maxCoeff(&dominant_component);
	if (direction[dominant_component] < 0.0) direction = -direction;
	return PrincipalAxisSolution{
		point(centroid), point(direction), eigenvalues.x(), eigenvalues.y(),
		eigenvalues.z()};
}

double projection(
	const VegetationMeasuredPoint3d &sample,
	const PrincipalAxisSolution &axis)
{
	return (vector(sample) - vector(axis.centroid_metres))
		.dot(vector(axis.direction));
}

double radial_distance(
	const VegetationMeasuredPoint3d &sample,
	const PrincipalAxisSolution &axis)
{
	const Eigen::Vector3d offset =
		vector(sample) - vector(axis.centroid_metres);
	const Eigen::Vector3d direction = vector(axis.direction);
	return (offset - direction * offset.dot(direction)).norm();
}

double median(std::vector<double> values)
{
	if (values.empty()) return 0.0;
	std::sort(values.begin(), values.end());
	const std::size_t middle = values.size() / 2u;
	if (values.size() % 2u != 0u) return values[middle];
	return (values[middle - 1u] + values[middle]) * 0.5;
}

VegetationPointCloudBounds3d bounds(
	const std::vector<IndexedWoodyPoint> &points)
{
	double minimum_x = std::numeric_limits<double>::infinity();
	double minimum_y = std::numeric_limits<double>::infinity();
	double minimum_z = std::numeric_limits<double>::infinity();
	double maximum_x = -std::numeric_limits<double>::infinity();
	double maximum_y = -std::numeric_limits<double>::infinity();
	double maximum_z = -std::numeric_limits<double>::infinity();
	for (const auto &sample : points) {
		minimum_x = std::min(minimum_x, sample.position_metres.x());
		minimum_y = std::min(minimum_y, sample.position_metres.y());
		minimum_z = std::min(minimum_z, sample.position_metres.z());
		maximum_x = std::max(maximum_x, sample.position_metres.x());
		maximum_y = std::max(maximum_y, sample.position_metres.y());
		maximum_z = std::max(maximum_z, sample.position_metres.z());
	}
	return VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d(minimum_x, minimum_y, minimum_z),
		VegetationMeasuredPoint3d(maximum_x, maximum_y, maximum_z));
}

}

VegetationWoodyAxisReconstructionReport
VegetationPrimaryWoodyAxisReconstructionService::reconstruct(
	const std::string &reconstruction_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const VegetationPointCloudReconstructionJob &job,
	const VegetationWoodyAxisReconstructionPolicy &policy) const
{
	std::vector<Issue> issues;
	if (!valid_policy(policy)) {
		issues.emplace_back(
			IssueCode::InvalidPolicy,
			"Primary woody-axis reconstruction policy is invalid.");
	}
	if (reconstruction_identifier.empty()) {
		issues.emplace_back(
			IssueCode::InvalidReconstructionIdentifier,
			"Primary woody-axis reconstruction identifier is empty.");
	}
	if (job.schemaVersion() != job_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedJobSchema,
			"Point-cloud reconstruction job schema is unsupported.");
	}
	if (job.target() != VegetationPointCloudReconstructionTarget::ShootArchitecture ||
	    admission_report.target() !=
		    VegetationPointCloudReconstructionTarget::ShootArchitecture) {
		issues.emplace_back(
			IssueCode::WrongReconstructionTarget,
			"Primary woody-axis reconstruction requires ShootArchitecture intent.");
	}
	if (job.algorithmIdentifier() != algorithm_identifier) {
		issues.emplace_back(
			IssueCode::UnsupportedAlgorithm,
			"Primary woody-axis reconstruction algorithm identifier is unsupported.");
	}
	if (job.datasetIdentifier() != dataset.datasetIdentifier() ||
	    admission_report.datasetIdentifier() != dataset.datasetIdentifier()) {
		issues.emplace_back(
			IssueCode::DatasetIdentityMismatch,
			"Dataset, admission report, and reconstruction job identities differ.");
	}
	if (job.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256() ||
	    admission_report.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch,
			"Dataset, admission report, and reconstruction job source hashes differ.");
	}
	if (!admission_report.admitted()) {
		issues.emplace_back(
			IssueCode::AdmissionRejected,
			"Point-cloud reconstruction admission report is rejected.");
	}
	if (!issues.empty()) {
		return report(std::move(issues), std::nullopt, std::nullopt);
	}

	std::vector<IndexedWoodyPoint> woody_points;
	std::vector<VegetationMeasuredPoint3d> initial_training_points;
	std::vector<VegetationMeasuredPoint3d> holdout_points;
	for (const auto &sample : dataset.points()) {
		if (sample.organClass() != VegetationPointCloudOrganClass::Woody) continue;
		const auto &position_metres = sample.positionMetres();
		if (!std::isfinite(position_metres.x()) ||
		    !std::isfinite(position_metres.y()) ||
		    !std::isfinite(position_metres.z())) {
			issues.emplace_back(
				IssueCode::InvalidPointExtent,
				"Woody point contains a non-finite coordinate.");
			continue;
		}
		woody_points.push_back(
			IndexedWoodyPoint{sample.sourceIndex(), position_metres});
		const std::size_t split_index =
			(sample.sourceIndex() % policy.holdoutDivisor() +
			 static_cast<std::size_t>(
				 job.deterministicSeed() % policy.holdoutDivisor())) %
			policy.holdoutDivisor();
		if (split_index == 0u) {
			holdout_points.push_back(position_metres);
		} else {
			initial_training_points.push_back(position_metres);
		}
	}
	if (!issues.empty()) {
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	if (woody_points.size() < policy.minimumWoodyPointCount()) {
		issues.emplace_back(
			IssueCode::InsufficientWoodyEvidence,
			"Woody point count is below the reconstruction boundary.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	if (initial_training_points.size() <
	    policy.minimumTrainingWoodyPointCount()) {
		issues.emplace_back(
			IssueCode::InsufficientTrainingEvidence,
			"Training split has insufficient woody points.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	const auto initial_axis = solve_principal_axis(initial_training_points);
	if (!initial_axis.has_value()) {
		issues.emplace_back(
			IssueCode::PrincipalAxisSolutionFailed,
			"Initial woody principal-axis solution failed.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	std::vector<double> initial_radial_distances;
	initial_radial_distances.reserve(initial_training_points.size());
	for (const auto &sample : initial_training_points) {
		initial_radial_distances.push_back(radial_distance(sample, *initial_axis));
	}
	const double median_radius = median(initial_radial_distances);
	std::vector<double> absolute_deviations;
	absolute_deviations.reserve(initial_radial_distances.size());
	for (const double distance : initial_radial_distances) {
		absolute_deviations.push_back(std::abs(distance - median_radius));
	}
	const double radial_mad = median(std::move(absolute_deviations));
	const double minimum_mad_scale =
		policy.minimumObservedRadiusMetres() * 0.05;
	const double radial_inlier_limit =
		median_radius + policy.radialOutlierMadMultiplier() *
		                    std::max(radial_mad, minimum_mad_scale);
	std::vector<VegetationMeasuredPoint3d> training_points;
	for (std::size_t index = 0u; index < initial_training_points.size(); ++index) {
		if (initial_radial_distances[index] <= radial_inlier_limit) {
			training_points.push_back(initial_training_points[index]);
		}
	}
	const std::size_t excluded_outlier_count =
		initial_training_points.size() - training_points.size();
	if (training_points.size() < policy.minimumTrainingWoodyPointCount()) {
		issues.emplace_back(
			IssueCode::InsufficientTrainingEvidence,
			"Radial trimming leaves insufficient woody training points.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	const auto principal_axis = solve_principal_axis(training_points);
	if (!principal_axis.has_value()) {
		issues.emplace_back(
			IssueCode::PrincipalAxisSolutionFailed,
			"Trimmed woody principal-axis solution failed.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	double minimum_projection = std::numeric_limits<double>::infinity();
	double maximum_projection = -std::numeric_limits<double>::infinity();
	for (const auto &sample : training_points) {
		const double sample_projection = projection(sample, *principal_axis);
		minimum_projection = std::min(minimum_projection, sample_projection);
		maximum_projection = std::max(maximum_projection, sample_projection);
	}
	const double axial_span = maximum_projection - minimum_projection;
	const double station_count_estimate =
		std::floor(axial_span / policy.axialStationSpacingMetres() + 0.5) + 1.0;
	if (!std::isfinite(station_count_estimate) || station_count_estimate < 2.0 ||
	    station_count_estimate >
		    static_cast<double>(std::numeric_limits<std::size_t>::max())) {
		issues.emplace_back(
			IssueCode::InsufficientAxialStations,
			"Woody evidence cannot produce a bounded axial station sequence.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	const std::size_t station_count =
		static_cast<std::size_t>(station_count_estimate);
	const std::size_t cylinder_limit = std::min(
		policy.maximumCylinderCount(), job.maximumOutputPrimitives());
	if (station_count - 1u > cylinder_limit) {
		issues.emplace_back(
			IssueCode::CylinderLimitExceeded,
			"Primary woody-axis cylinder count exceeds the job or policy boundary.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	std::vector<AxialStationAccumulator> station_accumulators(station_count);
	for (const auto &sample : training_points) {
		const double sample_projection = projection(sample, *principal_axis);
		const double station_coordinate =
			(sample_projection - minimum_projection) /
			policy.axialStationSpacingMetres();
		std::size_t station_index = static_cast<std::size_t>(
			std::llround(std::max(0.0, station_coordinate)));
		station_index = std::min(station_index, station_count - 1u);
		station_accumulators[station_index].projections_metres.push_back(
			sample_projection);
		station_accumulators[station_index].radial_distances_metres.push_back(
			radial_distance(sample, *principal_axis));
	}
	std::vector<VegetationWoodyAxisRadiusStation> radius_stations;
	radius_stations.reserve(station_count);
	const Eigen::Vector3d centroid = vector(principal_axis->centroid_metres);
	const Eigen::Vector3d direction = vector(principal_axis->direction);
	for (std::size_t station_index = 0u; station_index < station_count;
	     ++station_index) {
		const auto &accumulator = station_accumulators[station_index];
		if (accumulator.projections_metres.size() <
		    policy.minimumPointsPerAxialStation()) {
			issues.emplace_back(
				IssueCode::InsufficientAxialStations,
				"An axial radius station lacks the required woody support.");
			continue;
		}
		double projection_sum = 0.0;
		for (const double value : accumulator.projections_metres) {
			projection_sum += value;
		}
		const double mean_projection = projection_sum /
		                               accumulator.projections_metres.size();
		const double station_radius =
			median(accumulator.radial_distances_metres);
		if (station_radius < policy.minimumObservedRadiusMetres()) {
			issues.emplace_back(
				IssueCode::InsufficientRadialEvidence,
				"An axial radius station is below the measured-radius boundary.");
			continue;
		}
		radius_stations.emplace_back(
			reconstruction_identifier + ":station:" +
				std::to_string(station_index),
			station_index, mean_projection,
			point(centroid + direction * mean_projection), station_radius,
			accumulator.projections_metres.size());
	}
	if (!issues.empty()) {
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	if (radius_stations.size() < 2u) {
		issues.emplace_back(
			IssueCode::InsufficientAxialStations,
			"Primary woody-axis reconstruction requires at least two stations.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	std::vector<VegetationWoodyAxisCylinder> cylinders;
	cylinders.reserve(radius_stations.size() - 1u);
	for (std::size_t index = 1u; index < radius_stations.size(); ++index) {
		const auto &lower = radius_stations[index - 1u];
		const auto &upper = radius_stations[index];
		cylinders.emplace_back(
			reconstruction_identifier + ":cylinder:" +
				std::to_string(index - 1u),
			index - 1u, lower.centrePointMetres(), upper.centrePointMetres(),
			(lower.radiusMetres() + upper.radiusMetres()) * 0.5,
			lower.supportingPointCount() + upper.supportingPointCount());
	}
	VegetationWoodyAxisQualityEvidence quality_evidence(
		reconstruction_identifier, dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), dataset.points().size(),
		woody_points.size(), initial_training_points.size(),
		excluded_outlier_count, training_points, holdout_points,
		principal_axis->centroid_metres, principal_axis->direction,
		principal_axis->minor_variance,
		principal_axis->intermediate_variance,
		principal_axis->principal_variance, radius_stations, cylinders);
	const VegetationWoodyAxisQualityReport quality_report =
		VegetationWoodyAxisQualityEvaluationService().evaluate(
			quality_evidence, policy);
	if (!quality_report.acceptedForPrimaryAxisGeometry()) {
		issues.emplace_back(
			IssueCode::QualityGateRejected,
			"Primary woody-axis reconstruction failed its geometry quality gate.");
		return report(issues, quality_report, std::nullopt);
	}
	VegetationWoodyAxisReconstruction reconstruction(
		reconstruction_schema, reconstruction_identifier, job.jobIdentifier(),
		dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(),
		principal_axis->centroid_metres, principal_axis->direction,
		std::move(radius_stations), std::move(cylinders), bounds(woody_points),
		quality_report);
	return report({}, quality_report, std::move(reconstruction));
}
