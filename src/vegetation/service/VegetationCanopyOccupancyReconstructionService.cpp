#include "vegetation/service/VegetationCanopyOccupancyReconstructionService.h"

#include "vegetation/service/VegetationCanopyOccupancyQualityEvaluationService.h"

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

using Issue = VegetationCanopyOccupancyReconstructionIssue;
using IssueCode = VegetationCanopyOccupancyReconstructionIssueCode;

constexpr const char *reconstruction_schema =
	"ProGen3D-VegetationCanopyOccupancyReconstruction-v1";
constexpr const char *job_schema =
	"ProGen3D-VegetationPointCloudReconstructionJob-v1";
constexpr const char *algorithm_identifier =
	"ProGen3D-CanopyVoxelOccupancy-v1";

struct CellPointCounts
{
	std::size_t foliage = 0u;
	std::size_t woody = 0u;
	std::size_t unknown = 0u;
};

VegetationCanopyOccupancyReconstructionReport report(
	std::vector<Issue> issues,
	std::optional<VegetationCanopyOccupancyQualityReport> quality_report,
	std::optional<VegetationCanopyOccupancyReconstruction> reconstruction)
{
	return VegetationCanopyOccupancyReconstructionReport(
		std::move(issues), std::move(quality_report),
		std::move(reconstruction));
}

bool valid_policy(const VegetationCanopyOccupancyReconstructionPolicy &policy)
{
	return std::isfinite(policy.voxelEdgeLengthMetres()) &&
	       policy.voxelEdgeLengthMetres() > 0.0 &&
	       policy.minimumFoliagePointsPerCell() > 0u &&
	       policy.maximumOccupiedCellCount() > 0u &&
	       policy.holdoutDivisor() >= 2u &&
	       std::isfinite(policy.minimumHoldoutRecall()) &&
	       policy.minimumHoldoutRecall() >= 0.0 &&
	       policy.minimumHoldoutRecall() <= 1.0 &&
	       std::isfinite(policy.minimumOrganLabelFraction()) &&
	       policy.minimumOrganLabelFraction() >= 0.0 &&
	       policy.minimumOrganLabelFraction() <= 1.0;
}

std::optional<std::size_t> voxel_axis_index(
	double coordinate,
	double origin,
	double voxel_edge_length_metres)
{
	const double relative = (coordinate - origin) / voxel_edge_length_metres;
	if (!std::isfinite(relative) || relative < -1.0e-9 ||
	    relative > static_cast<double>(std::numeric_limits<std::size_t>::max())) {
		return std::nullopt;
	}
	return static_cast<std::size_t>(std::floor(std::max(0.0, relative)));
}

std::optional<VegetationCanopyVoxelIndex> voxel_index(
	const VegetationMeasuredPoint3d &position,
	const VegetationMeasuredPoint3d &origin,
	double voxel_edge_length_metres)
{
	const auto x =
		voxel_axis_index(position.x(), origin.x(), voxel_edge_length_metres);
	const auto y =
		voxel_axis_index(position.y(), origin.y(), voxel_edge_length_metres);
	const auto z =
		voxel_axis_index(position.z(), origin.z(), voxel_edge_length_metres);
	if (!x.has_value() || !y.has_value() || !z.has_value()) return std::nullopt;
	return VegetationCanopyVoxelIndex(*x, *y, *z);
}

VegetationMeasuredPoint3d cell_centre(
	const VegetationCanopyVoxelIndex &index,
	const VegetationMeasuredPoint3d &origin,
	double edge_length)
{
	return VegetationMeasuredPoint3d(
		origin.x() + (static_cast<double>(index.xIndex()) + 0.5) * edge_length,
		origin.y() + (static_cast<double>(index.yIndex()) + 0.5) * edge_length,
		origin.z() + (static_cast<double>(index.zIndex()) + 0.5) * edge_length);
}

VegetationPointCloudBounds3d bounds(
	const std::vector<VegetationMeasuredPoint3d> &positions)
{
	double minimum_x = positions.front().x();
	double minimum_y = positions.front().y();
	double minimum_z = positions.front().z();
	double maximum_x = minimum_x;
	double maximum_y = minimum_y;
	double maximum_z = minimum_z;
	for (const auto &position : positions) {
		minimum_x = std::min(minimum_x, position.x());
		minimum_y = std::min(minimum_y, position.y());
		minimum_z = std::min(minimum_z, position.z());
		maximum_x = std::max(maximum_x, position.x());
		maximum_y = std::max(maximum_y, position.y());
		maximum_z = std::max(maximum_z, position.z());
	}
	return VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d(minimum_x, minimum_y, minimum_z),
		VegetationMeasuredPoint3d(maximum_x, maximum_y, maximum_z));
}

}

VegetationCanopyOccupancyReconstructionReport
VegetationCanopyOccupancyReconstructionService::reconstruct(
	const std::string &reconstruction_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const VegetationPointCloudReconstructionJob &job,
	const VegetationCanopyOccupancyReconstructionPolicy &policy) const
{
	std::vector<Issue> issues;
	if (!valid_policy(policy)) {
		issues.emplace_back(
			IssueCode::InvalidPolicy,
			"Canopy occupancy reconstruction policy is invalid.");
	}
	if (reconstruction_identifier.empty()) {
		issues.emplace_back(
			IssueCode::InvalidReconstructionIdentifier,
			"Canopy occupancy reconstruction requires an identifier.");
	}
	if (job.schemaVersion() != job_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedJobSchema,
			"Point-cloud reconstruction job schema is unsupported.");
	}
	if (job.target() != VegetationPointCloudReconstructionTarget::CanopyOptics ||
	    admission_report.target() !=
		    VegetationPointCloudReconstructionTarget::CanopyOptics) {
		issues.emplace_back(
			IssueCode::WrongReconstructionTarget,
			"Canopy occupancy reconstruction requires the CanopyOptics target.");
	}
	if (job.algorithmIdentifier() != algorithm_identifier) {
		issues.emplace_back(
			IssueCode::UnsupportedAlgorithm,
			"Canopy occupancy reconstruction algorithm identifier is unsupported.");
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

	const VegetationMeasuredPoint3d origin = dataset.observedBounds().minimum();
	std::map<VegetationCanopyVoxelIndex, CellPointCounts> all_cell_counts;
	std::map<VegetationCanopyVoxelIndex, std::size_t> training_foliage_counts;
	std::vector<VegetationCanopyVoxelIndex> holdout_foliage_cells;
	std::vector<VegetationMeasuredPoint3d> foliage_positions;
	std::size_t organ_labeled_point_count = 0u;
	for (const auto &point : dataset.points()) {
		const auto index = voxel_index(
			point.positionMetres(), origin, policy.voxelEdgeLengthMetres());
		if (!index.has_value()) {
			issues.emplace_back(
				IssueCode::InvalidPointExtent,
				"Point lies outside the finite voxel index range.");
			continue;
		}
		auto &counts = all_cell_counts[*index];
		switch (point.organClass()) {
		case VegetationPointCloudOrganClass::Woody:
			++counts.woody;
			++organ_labeled_point_count;
			break;
		case VegetationPointCloudOrganClass::Foliage: {
			++counts.foliage;
			++organ_labeled_point_count;
			foliage_positions.push_back(point.positionMetres());
			const std::size_t split_index =
				(point.sourceIndex() % policy.holdoutDivisor() +
				 static_cast<std::size_t>(
					 job.deterministicSeed() % policy.holdoutDivisor())) %
				policy.holdoutDivisor();
			if (split_index == 0u) {
				holdout_foliage_cells.push_back(*index);
			} else {
				++training_foliage_counts[*index];
			}
			break;
		}
		case VegetationPointCloudOrganClass::Unknown: ++counts.unknown; break;
		}
	}
	if (!issues.empty()) {
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	if (foliage_positions.empty()) {
		issues.emplace_back(
			IssueCode::InsufficientFoliageEvidence,
			"Canopy occupancy reconstruction requires foliage-labeled points.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}

	std::vector<VegetationCanopyVoxelIndex> training_occupied_cells;
	for (const auto &entry : training_foliage_counts) {
		if (entry.second >= policy.minimumFoliagePointsPerCell()) {
			training_occupied_cells.push_back(entry.first);
		}
	}
	std::vector<VegetationCanopyOccupancyCell> reconstructed_cells;
	for (const auto &entry : all_cell_counts) {
		if (entry.second.foliage < policy.minimumFoliagePointsPerCell()) continue;
		reconstructed_cells.emplace_back(
			entry.first,
			cell_centre(entry.first, origin, policy.voxelEdgeLengthMetres()),
			entry.second.foliage, entry.second.woody, entry.second.unknown);
	}
	const std::size_t occupied_cell_limit = std::min(
		policy.maximumOccupiedCellCount(), job.maximumOutputPrimitives());
	if (reconstructed_cells.size() > occupied_cell_limit) {
		issues.emplace_back(
			IssueCode::OccupiedCellLimitExceeded,
			"Canopy occupancy cell count exceeds the job or policy boundary.");
		return report(std::move(issues), std::nullopt, std::nullopt);
	}
	const VegetationPointCloudBounds3d crown_bounds = bounds(foliage_positions);
	VegetationCanopyOccupancyQualityEvidence quality_evidence(
		reconstruction_identifier, dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), origin,
		policy.voxelEdgeLengthMetres(), dataset.points().size(),
		organ_labeled_point_count, foliage_positions.size(),
		std::move(training_occupied_cells), std::move(holdout_foliage_cells),
		reconstructed_cells, crown_bounds);
	const VegetationCanopyOccupancyQualityReport quality_report =
		VegetationCanopyOccupancyQualityEvaluationService().evaluate(
			quality_evidence, policy);
	if (!quality_report.acceptedForGeometryUse()) {
		issues.emplace_back(
			IssueCode::QualityGateRejected,
			"Canopy occupancy reconstruction failed its geometry quality gate.");
		return report(issues, quality_report, std::nullopt);
	}
	VegetationCanopyOccupancyReconstruction reconstruction(
		reconstruction_schema, reconstruction_identifier, job.jobIdentifier(),
		dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(), origin,
		policy.voxelEdgeLengthMetres(), std::move(reconstructed_cells),
		crown_bounds, quality_report);
	return report({}, quality_report, std::move(reconstruction));
}
