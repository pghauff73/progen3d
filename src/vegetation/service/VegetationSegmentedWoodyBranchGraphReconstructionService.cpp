#include "vegetation/service/VegetationSegmentedWoodyBranchGraphReconstructionService.h"

#include "vegetation/service/VegetationWoodyBranchGraphQualityEvaluationService.h"
#include "vegetation/service/VegetationWoodyPointSegmentationValidationService.h"

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationWoodyBranchGraphReconstructionIssue;
using IssueCode = VegetationWoodyBranchGraphReconstructionIssueCode;

constexpr const char *reconstruction_schema =
	"ProGen3D-VegetationWoodyBranchGraphReconstruction-v1";
constexpr const char *job_schema =
	"ProGen3D-VegetationPointCloudReconstructionJob-v1";
constexpr const char *algorithm_identifier =
	"ProGen3D-SegmentedWoodyBranchGraph-v1";

struct SegmentedPoint
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

struct ClosestParentCylinder
{
	std::string cylinder_identifier;
	VegetationMeasuredPoint3d centreline_point_metres{0.0, 0.0, 0.0};
	double centreline_distance_metres = std::numeric_limits<double>::infinity();
	double surface_gap_metres = std::numeric_limits<double>::infinity();
};

struct FittedAxisResult
{
	VegetationWoodyBranchAxis axis;
	VegetationWoodyBranchAxisQualityEvidence quality;
	std::optional<VegetationWoodyBranchConnection> connection;

	FittedAxisResult(
		VegetationWoodyBranchAxis fitted_axis,
		VegetationWoodyBranchAxisQualityEvidence quality_evidence,
		std::optional<VegetationWoodyBranchConnection> branch_connection)
		: axis(std::move(fitted_axis)),
		  quality(std::move(quality_evidence)),
		  connection(std::move(branch_connection))
	{
	}
};

struct AxisFitFailure
{
	IssueCode code = IssueCode::AxisFitFailed;
	std::string message;
};

struct AxisFitOutcome
{
	std::optional<FittedAxisResult> result;
	std::optional<AxisFitFailure> failure;
};

VegetationWoodyBranchGraphReconstructionReport report(
	std::vector<Issue> issues,
	std::optional<VegetationWoodyPointSegmentationValidationReport>
		segmentation_report,
	std::optional<VegetationWoodyBranchGraphQualityReport> quality_report,
	std::optional<VegetationWoodyBranchGraphReconstruction> reconstruction)
{
	return VegetationWoodyBranchGraphReconstructionReport(
		std::move(issues), std::move(segmentation_report),
		std::move(quality_report), std::move(reconstruction));
}

bool fraction(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool valid_policy(const VegetationWoodyBranchGraphReconstructionPolicy &policy)
{
	return policy.minimumAxisPointCount() > 0u &&
	       policy.minimumTrainingAxisPointCount() > 2u &&
	       policy.minimumPointsPerAxialStation() > 0u &&
	       policy.maximumAxisCount() > 1u &&
	       policy.maximumTotalCylinderCount() > 0u &&
	       policy.holdoutDivisor() > 1u &&
	       fraction(policy.minimumAssignmentConfidence()) &&
	       fraction(policy.maximumUnassignedWoodyFraction()) &&
	       fraction(policy.minimumPrincipalVarianceFraction()) &&
	       std::isfinite(policy.maximumTrainingSurfaceRmseMetres()) &&
	       policy.maximumTrainingSurfaceRmseMetres() >= 0.0 &&
	       std::isfinite(policy.maximumHoldoutSurfaceRmseMetres()) &&
	       policy.maximumHoldoutSurfaceRmseMetres() >= 0.0 &&
	       std::isfinite(policy.maximumHoldoutSurfaceResidualMetres()) &&
	       policy.maximumHoldoutSurfaceResidualMetres() >= 0.0 &&
	       fraction(policy.minimumHoldoutSurfaceCoverageFraction()) &&
	       std::isfinite(policy.maximumAttachmentSurfaceGapMetres()) &&
	       policy.maximumAttachmentSurfaceGapMetres() >= 0.0 &&
	       fraction(policy.maximumTaperViolationFraction()) &&
	       std::isfinite(policy.minimumObservedRadiusMetres()) &&
	       policy.minimumObservedRadiusMetres() > 0.0 &&
	       std::isfinite(policy.minimumAxisLengthMetres()) &&
	       policy.minimumAxisLengthMetres() > 0.0;
}

Eigen::Vector3d vector(const VegetationMeasuredPoint3d &point)
{
	return Eigen::Vector3d(point.x(), point.y(), point.z());
}

VegetationMeasuredPoint3d point(const Eigen::Vector3d &value)
{
	return VegetationMeasuredPoint3d(value.x(), value.y(), value.z());
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

std::uint64_t stable_identifier_hash(const std::string &identifier)
{
	std::uint64_t hash = 1469598103934665603ull;
	for (const unsigned char character : identifier) {
		hash ^= character;
		hash *= 1099511628211ull;
	}
	return hash;
}

VegetationMeasuredPoint3d closest_point_on_segment(
	const VegetationMeasuredPoint3d &sample,
	const VegetationMeasuredPoint3d &start,
	const VegetationMeasuredPoint3d &end)
{
	const Eigen::Vector3d start_vector = vector(start);
	const Eigen::Vector3d direction = vector(end) - start_vector;
	const double length_squared = direction.squaredNorm();
	if (length_squared <= 0.0) return start;
	const double factor = std::clamp(
		(vector(sample) - start_vector).dot(direction) / length_squared,
		0.0, 1.0);
	return point(start_vector + direction * factor);
}

double distance(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	return (vector(first) - vector(second)).norm();
}

ClosestParentCylinder closest_parent_cylinder(
	const VegetationMeasuredPoint3d &sample,
	const VegetationWoodyBranchAxis &parent_axis)
{
	ClosestParentCylinder closest;
	for (const auto &cylinder : parent_axis.cylinders()) {
		const VegetationMeasuredPoint3d centreline_point =
			closest_point_on_segment(
				sample, cylinder.startPointMetres(), cylinder.endPointMetres());
		const double centreline_distance = distance(sample, centreline_point);
		if (centreline_distance >= closest.centreline_distance_metres) continue;
		closest.cylinder_identifier = cylinder.cylinderIdentifier();
		closest.centreline_point_metres = centreline_point;
		closest.centreline_distance_metres = centreline_distance;
		closest.surface_gap_metres =
			std::max(0.0, centreline_distance - cylinder.radiusMetres());
	}
	return closest;
}

double interpolated_radius(
	double sample_projection,
	const std::vector<VegetationWoodyAxisRadiusStation> &stations)
{
	if (stations.empty()) return 0.0;
	if (sample_projection <= stations.front().axialProjectionMetres()) {
		return stations.front().radiusMetres();
	}
	if (sample_projection >= stations.back().axialProjectionMetres()) {
		return stations.back().radiusMetres();
	}
	for (std::size_t index = 1u; index < stations.size(); ++index) {
		const auto &upper = stations[index];
		if (sample_projection > upper.axialProjectionMetres()) continue;
		const auto &lower = stations[index - 1u];
		const double span = upper.axialProjectionMetres() -
		                    lower.axialProjectionMetres();
		const double factor = span <= 0.0
			                      ? 0.0
			                      : std::clamp(
				                        (sample_projection -
				                         lower.axialProjectionMetres()) /
					                        span,
				                        0.0, 1.0);
		return lower.radiusMetres() +
		       (upper.radiusMetres() - lower.radiusMetres()) * factor;
	}
	return stations.back().radiusMetres();
}

double surface_residual(
	const VegetationMeasuredPoint3d &sample,
	const PrincipalAxisSolution &axis,
	const std::vector<VegetationWoodyAxisRadiusStation> &stations)
{
	const double sample_projection = projection(sample, axis);
	return std::abs(
		radial_distance(sample, axis) -
		interpolated_radius(sample_projection, stations));
}

double surface_rmse(
	const std::vector<VegetationMeasuredPoint3d> &points,
	const PrincipalAxisSolution &axis,
	const std::vector<VegetationWoodyAxisRadiusStation> &stations)
{
	if (points.empty() || stations.empty()) {
		return std::numeric_limits<double>::infinity();
	}
	double squared_error_sum = 0.0;
	for (const auto &sample : points) {
		const double residual = surface_residual(sample, axis, stations);
		squared_error_sum += residual * residual;
	}
	return std::sqrt(squared_error_sum / static_cast<double>(points.size()));
}

VegetationPointCloudBounds3d woody_bounds(
	const VegetationPointCloudDataset &dataset)
{
	double minimum_x = std::numeric_limits<double>::infinity();
	double minimum_y = std::numeric_limits<double>::infinity();
	double minimum_z = std::numeric_limits<double>::infinity();
	double maximum_x = -std::numeric_limits<double>::infinity();
	double maximum_y = -std::numeric_limits<double>::infinity();
	double maximum_z = -std::numeric_limits<double>::infinity();
	for (const auto &sample : dataset.points()) {
		if (sample.organClass() != VegetationPointCloudOrganClass::Woody) continue;
		minimum_x = std::min(minimum_x, sample.positionMetres().x());
		minimum_y = std::min(minimum_y, sample.positionMetres().y());
		minimum_z = std::min(minimum_z, sample.positionMetres().z());
		maximum_x = std::max(maximum_x, sample.positionMetres().x());
		maximum_y = std::max(maximum_y, sample.positionMetres().y());
		maximum_z = std::max(maximum_z, sample.positionMetres().z());
	}
	return VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d(minimum_x, minimum_y, minimum_z),
		VegetationMeasuredPoint3d(maximum_x, maximum_y, maximum_z));
}

AxisFitOutcome fit_axis(
	const std::string &reconstruction_identifier,
	const VegetationWoodyAxisSegmentDefinition &definition,
	const std::vector<SegmentedPoint> &segment_points,
	std::uint64_t deterministic_seed,
	const VegetationWoodyBranchGraphReconstructionPolicy &policy,
	const VegetationWoodyBranchAxis *parent_axis)
{
	if (segment_points.size() < policy.minimumAxisPointCount()) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::InsufficientAxisEvidence,
				"Axis point count is below the graph reconstruction boundary."}};
	}
	std::vector<VegetationMeasuredPoint3d> training_points;
	std::vector<VegetationMeasuredPoint3d> holdout_points;
	const std::size_t split_offset = static_cast<std::size_t>(
		(deterministic_seed +
		 stable_identifier_hash(definition.axisIdentifier())) %
		policy.holdoutDivisor());
	for (const auto &sample : segment_points) {
		if ((sample.source_index % policy.holdoutDivisor() + split_offset) %
		        policy.holdoutDivisor() ==
		    0u) {
			holdout_points.push_back(sample.position_metres);
		} else {
			training_points.push_back(sample.position_metres);
		}
	}
	if (training_points.size() < policy.minimumTrainingAxisPointCount() ||
	    holdout_points.empty()) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::InsufficientAxisEvidence,
				"Axis training or holdout split is insufficient."}};
	}
	const auto solved_axis = solve_principal_axis(training_points);
	if (!solved_axis.has_value()) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::AxisFitFailed,
				"Axis covariance solution failed."}};
	}
	PrincipalAxisSolution axis = *solved_axis;
	auto projection_range = [&](const PrincipalAxisSolution &candidate) {
		double minimum_projection = std::numeric_limits<double>::infinity();
		double maximum_projection = -std::numeric_limits<double>::infinity();
		for (const auto &sample : training_points) {
			const double value = projection(sample, candidate);
			minimum_projection = std::min(minimum_projection, value);
			maximum_projection = std::max(maximum_projection, value);
		}
		return std::make_pair(minimum_projection, maximum_projection);
	};
	auto range = projection_range(axis);
	if (parent_axis != nullptr) {
		const Eigen::Vector3d centroid = vector(axis.centroid_metres);
		const Eigen::Vector3d direction = vector(axis.direction);
		const auto minimum_parent = closest_parent_cylinder(
			point(centroid + direction * range.first), *parent_axis);
		const auto maximum_parent = closest_parent_cylinder(
			point(centroid + direction * range.second), *parent_axis);
		if (maximum_parent.centreline_distance_metres <
		    minimum_parent.centreline_distance_metres) {
			axis.direction = point(-direction);
			range = projection_range(axis);
		}
	}
	const double axial_length = range.second - range.first;
	if (axial_length < policy.minimumAxisLengthMetres()) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::InsufficientAxisEvidence,
				"Axis length is below the graph reconstruction boundary."}};
	}
	const double station_count_estimate =
		std::floor(
			axial_length / definition.axialStationSpacingMetres() + 0.5) +
		1.0;
	if (!std::isfinite(station_count_estimate) || station_count_estimate < 2.0 ||
	    station_count_estimate >
		    static_cast<double>(std::numeric_limits<std::size_t>::max())) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::InsufficientAxialStations,
				"Axis cannot produce a bounded station sequence."}};
	}
	const std::size_t station_count =
		static_cast<std::size_t>(station_count_estimate);
	std::vector<AxialStationAccumulator> accumulators(station_count);
	for (const auto &sample : training_points) {
		const double sample_projection = projection(sample, axis);
		const double coordinate =
			(sample_projection - range.first) /
			definition.axialStationSpacingMetres();
		std::size_t station_index = static_cast<std::size_t>(
			std::llround(std::max(0.0, coordinate)));
		station_index = std::min(station_index, station_count - 1u);
		accumulators[station_index].projections_metres.push_back(
			sample_projection);
		accumulators[station_index].radial_distances_metres.push_back(
			radial_distance(sample, axis));
	}
	std::vector<VegetationWoodyAxisRadiusStation> stations;
	stations.reserve(station_count);
	const Eigen::Vector3d centroid = vector(axis.centroid_metres);
	const Eigen::Vector3d direction = vector(axis.direction);
	for (std::size_t station_index = 0u; station_index < station_count;
	     ++station_index) {
		const auto &accumulator = accumulators[station_index];
		if (accumulator.projections_metres.size() <
		    policy.minimumPointsPerAxialStation()) {
			return AxisFitOutcome{
				std::nullopt,
				AxisFitFailure{
					IssueCode::InsufficientAxialStations,
					"Axis station lacks the required point support."}};
		}
		double projection_sum = 0.0;
		for (const double value : accumulator.projections_metres) {
			projection_sum += value;
		}
		const double station_projection = projection_sum /
		                                  accumulator.projections_metres.size();
		const double station_radius =
			median(accumulator.radial_distances_metres);
		if (station_radius < policy.minimumObservedRadiusMetres()) {
			return AxisFitOutcome{
				std::nullopt,
				AxisFitFailure{
					IssueCode::InsufficientRadialEvidence,
					"Axis station radius is below the measured boundary."}};
		}
		stations.emplace_back(
			reconstruction_identifier + ":" + definition.axisIdentifier() +
				":station:" + std::to_string(station_index),
			station_index, station_projection,
			point(centroid + direction * station_projection), station_radius,
			accumulator.projections_metres.size());
	}

	std::optional<ClosestParentCylinder> parent_connection;
	if (parent_axis != nullptr) {
		parent_connection = closest_parent_cylinder(
			stations.front().centrePointMetres(), *parent_axis);
		if (parent_connection->cylinder_identifier.empty()) {
			return AxisFitOutcome{
				std::nullopt,
				AxisFitFailure{
					IssueCode::ParentAxisUnavailable,
					"Parent axis contains no attachable cylinder."}};
		}
	}
	std::vector<VegetationWoodyBranchCylinder> cylinders;
	cylinders.reserve(stations.size() - 1u);
	for (std::size_t index = 1u; index < stations.size(); ++index) {
		const std::string cylinder_identifier =
			reconstruction_identifier + ":" + definition.axisIdentifier() +
			":cylinder:" + std::to_string(index - 1u);
		const std::string parent_identifier = index == 1u
			                                      ? (parent_connection.has_value()
				                                         ? parent_connection
					                                           ->cylinder_identifier
				                                         : std::string())
			                                      : cylinders.back()
				                                        .cylinderIdentifier();
		cylinders.emplace_back(
			cylinder_identifier, parent_identifier, definition.axisIdentifier(),
			index - 1u, definition.branchOrder(),
			stations[index - 1u].centrePointMetres(),
			stations[index].centrePointMetres(),
			(stations[index - 1u].radiusMetres() +
			 stations[index].radiusMetres()) *
				0.5,
			stations[index - 1u].supportingPointCount() +
				stations[index].supportingPointCount());
	}
	if (cylinders.empty()) {
		return AxisFitOutcome{
			std::nullopt,
			AxisFitFailure{
				IssueCode::InsufficientAxialStations,
				"Axis reconstruction produced no cylinders."}};
	}
	const double variance_sum = axis.minor_variance +
	                            axis.intermediate_variance +
	                            axis.principal_variance;
	const double principal_variance_fraction = variance_sum > 0.0
		                                           ? axis.principal_variance /
			                                             variance_sum
		                                           : 0.0;
	const double training_rmse = surface_rmse(training_points, axis, stations);
	const double holdout_rmse = surface_rmse(holdout_points, axis, stations);
	std::size_t covered_holdout_count = 0u;
	for (const auto &sample : holdout_points) {
		if (surface_residual(sample, axis, stations) <=
		    policy.maximumHoldoutSurfaceResidualMetres()) {
			++covered_holdout_count;
		}
	}
	const double holdout_coverage = static_cast<double>(covered_holdout_count) /
	                                holdout_points.size();
	std::size_t taper_violation_count = 0u;
	for (std::size_t index = 1u; index < stations.size(); ++index) {
		if (stations[index].radiusMetres() >
		    stations[index - 1u].radiusMetres() + 1e-9) {
			++taper_violation_count;
		}
	}
	const double taper_violation_fraction = static_cast<double>(
		                                             taper_violation_count) /
	                                         (stations.size() - 1u);
	VegetationWoodyBranchAxis fitted_axis(
		definition.axisIdentifier(), definition.parentAxisIdentifier(),
		definition.branchOrder(), axis.centroid_metres, axis.direction,
		stations, cylinders);
	VegetationWoodyBranchAxisQualityEvidence quality(
		definition.axisIdentifier(), training_points.size(),
		holdout_points.size(), principal_variance_fraction, training_rmse,
		holdout_rmse, holdout_coverage, taper_violation_fraction);
	std::optional<VegetationWoodyBranchConnection> connection;
	if (parent_connection.has_value()) {
		connection = VegetationWoodyBranchConnection(
			definition.parentAxisIdentifier(), definition.axisIdentifier(),
			parent_connection->cylinder_identifier,
			cylinders.front().cylinderIdentifier(),
			parent_connection->centreline_point_metres,
			parent_connection->surface_gap_metres);
	}
	return AxisFitOutcome{
		FittedAxisResult(
			std::move(fitted_axis), std::move(quality),
			std::move(connection)),
		std::nullopt};
}

}

VegetationWoodyBranchGraphReconstructionReport
VegetationSegmentedWoodyBranchGraphReconstructionService::reconstruct(
	const std::string &reconstruction_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const VegetationPointCloudReconstructionJob &job,
	const VegetationWoodyPointSegmentation &segmentation,
	const VegetationWoodyBranchGraphReconstructionPolicy &policy) const
{
	std::vector<Issue> issues;
	if (!valid_policy(policy)) {
		issues.emplace_back(
			IssueCode::InvalidPolicy, std::string(),
			"Woody branch graph reconstruction policy is invalid.");
	}
	if (reconstruction_identifier.empty()) {
		issues.emplace_back(
			IssueCode::InvalidReconstructionIdentifier, std::string(),
			"Woody branch graph reconstruction identifier is empty.");
	}
	if (job.schemaVersion() != job_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedJobSchema, std::string(),
			"Point-cloud reconstruction job schema is unsupported.");
	}
	if (job.target() != VegetationPointCloudReconstructionTarget::ShootArchitecture ||
	    admission_report.target() !=
		    VegetationPointCloudReconstructionTarget::ShootArchitecture) {
		issues.emplace_back(
			IssueCode::WrongReconstructionTarget, std::string(),
			"Woody branch graph reconstruction requires ShootArchitecture intent.");
	}
	if (job.algorithmIdentifier() != algorithm_identifier) {
		issues.emplace_back(
			IssueCode::UnsupportedAlgorithm, std::string(),
			"Woody branch graph reconstruction algorithm is unsupported.");
	}
	if (job.datasetIdentifier() != dataset.datasetIdentifier() ||
	    admission_report.datasetIdentifier() != dataset.datasetIdentifier() ||
	    segmentation.datasetIdentifier() != dataset.datasetIdentifier()) {
		issues.emplace_back(
			IssueCode::DatasetIdentityMismatch, std::string(),
			"Dataset, admission, segmentation, and job identities differ.");
	}
	if (job.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256() ||
	    admission_report.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256() ||
	    segmentation.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch, std::string(),
			"Dataset, admission, segmentation, and job source hashes differ.");
	}
	if (!admission_report.admitted()) {
		issues.emplace_back(
			IssueCode::AdmissionRejected, std::string(),
			"Point-cloud reconstruction admission report is rejected.");
	}
	if (!issues.empty()) {
		return report(
			std::move(issues), std::nullopt, std::nullopt, std::nullopt);
	}
	const auto segmentation_report =
		VegetationWoodyPointSegmentationValidationService().validate(
			segmentation, dataset);
	if (!segmentation_report.acceptedForGraphConstruction()) {
		issues.emplace_back(
			IssueCode::SegmentationRejected, std::string(),
			"Woody point segmentation failed validation.");
		return report(
			std::move(issues), segmentation_report, std::nullopt,
			std::nullopt);
	}
	if (!segmentation.completeForObservedWoodyPoints()) {
		issues.emplace_back(
			IssueCode::SegmentationIncomplete, std::string(),
			"Observed woody graph reconstruction requires complete segmentation.");
		return report(
			std::move(issues), segmentation_report, std::nullopt,
			std::nullopt);
	}
	if (segmentation.axisDefinitions().size() > policy.maximumAxisCount()) {
		issues.emplace_back(
			IssueCode::AxisLimitExceeded, std::string(),
			"Woody axis count exceeds the graph reconstruction boundary.");
		return report(
			std::move(issues), segmentation_report, std::nullopt,
			std::nullopt);
	}

	std::map<std::size_t, const VegetationPointCloudPointRecord *> points;
	for (const auto &sample : dataset.points()) {
		points.emplace(sample.sourceIndex(), &sample);
	}
	std::map<std::string, std::vector<SegmentedPoint>> points_by_axis;
	for (const auto &assignment : segmentation.pointAssignments()) {
		const auto point_record = points.find(assignment.sourcePointIndex());
		if (point_record == points.end()) continue;
		points_by_axis[assignment.axisIdentifier()].push_back(SegmentedPoint{
			assignment.sourcePointIndex(), point_record->second->positionMetres()});
	}
	std::vector<const VegetationWoodyAxisSegmentDefinition *> definitions;
	for (const auto &definition : segmentation.axisDefinitions()) {
		definitions.push_back(&definition);
	}
	std::sort(
		definitions.begin(), definitions.end(),
		[](const auto *left, const auto *right) {
			if (left->branchOrder() != right->branchOrder()) {
				return left->branchOrder() < right->branchOrder();
			}
			return left->axisIdentifier() < right->axisIdentifier();
		});
	std::vector<VegetationWoodyBranchAxis> axes;
	std::vector<VegetationWoodyBranchConnection> connections;
	std::vector<VegetationWoodyBranchAxisQualityEvidence> axis_quality;
	std::map<std::string, std::size_t> axis_indices;
	std::size_t total_cylinder_count = 0u;
	for (const auto *definition : definitions) {
		const VegetationWoodyBranchAxis *parent_axis = nullptr;
		if (!definition->isRootAxis()) {
			const auto parent_index = axis_indices.find(
				definition->parentAxisIdentifier());
			if (parent_index == axis_indices.end()) {
				issues.emplace_back(
					IssueCode::ParentAxisUnavailable,
					definition->axisIdentifier(),
					"Parent woody axis was not reconstructed.");
				break;
			}
			parent_axis = &axes[parent_index->second];
		}
		const AxisFitOutcome fitted = fit_axis(
			reconstruction_identifier, *definition,
			points_by_axis[definition->axisIdentifier()],
			job.deterministicSeed(), policy, parent_axis);
		if (!fitted.result.has_value()) {
			issues.emplace_back(
				fitted.failure->code, definition->axisIdentifier(),
				fitted.failure->message);
			break;
		}
		total_cylinder_count += fitted.result->axis.cylinders().size();
		const std::size_t cylinder_limit = std::min(
			policy.maximumTotalCylinderCount(), job.maximumOutputPrimitives());
		if (total_cylinder_count > cylinder_limit) {
			issues.emplace_back(
				IssueCode::CylinderLimitExceeded,
				definition->axisIdentifier(),
				"Woody cylinder count exceeds the job or policy boundary.");
			break;
		}
		axis_indices.emplace(definition->axisIdentifier(), axes.size());
		axes.push_back(fitted.result->axis);
		axis_quality.push_back(fitted.result->quality);
		if (fitted.result->connection.has_value()) {
			connections.push_back(*fitted.result->connection);
		}
	}
	if (!issues.empty()) {
		return report(
			std::move(issues), segmentation_report, std::nullopt,
			std::nullopt);
	}
	VegetationWoodyBranchGraphQualityEvidence quality_evidence(
		reconstruction_identifier, dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), segmentation_report,
		axes, connections, axis_quality);
	const auto quality_report =
		VegetationWoodyBranchGraphQualityEvaluationService().evaluate(
			quality_evidence, policy);
	if (!quality_report.acceptedForObservedWoodyGraph()) {
		issues.emplace_back(
			IssueCode::QualityGateRejected, std::string(),
			"Woody branch graph failed its quality boundary.");
		return report(
			std::move(issues), segmentation_report, quality_report,
			std::nullopt);
	}
	VegetationWoodyBranchGraphReconstruction reconstruction(
		reconstruction_schema, reconstruction_identifier, job.jobIdentifier(),
		dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(),
		segmentation.segmentationIdentifier(),
		segmentation.segmentationAlgorithmIdentifier(),
		segmentation.rootAxisIdentifier(), std::move(axes),
		std::move(connections), woody_bounds(dataset), quality_report);
	return report(
		{}, segmentation_report, quality_report, std::move(reconstruction));
}
