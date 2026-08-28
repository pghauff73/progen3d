#include "vegetation/service/VegetationCanopyOccupancyQualityEvaluationService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

std::size_t index_distance(std::size_t first, std::size_t second)
{
	return first > second ? first - second : second - first;
}

bool has_training_neighbor(
	const VegetationCanopyVoxelIndex &holdout,
	const std::vector<VegetationCanopyVoxelIndex> &training,
	std::size_t radius)
{
	return std::any_of(
		training.begin(), training.end(), [&](const auto &candidate) {
			return index_distance(holdout.xIndex(), candidate.xIndex()) <= radius &&
			       index_distance(holdout.yIndex(), candidate.yIndex()) <= radius &&
			       index_distance(holdout.zIndex(), candidate.zIndex()) <= radius;
		});
}

struct ProjectedOccupancyMetrics
{
	double occupancy_area_square_metres = 0.0;
	double gap_proxy = 0.0;
};

template <typename Projection>
ProjectedOccupancyMetrics projected_metrics(
	const std::vector<VegetationCanopyOccupancyCell> &cells,
	double voxel_edge_length_metres,
	Projection projection)
{
	std::set<std::pair<std::size_t, std::size_t>> occupied;
	for (const auto &cell : cells) occupied.insert(projection(cell.index()));
	if (occupied.empty()) return {};
	std::size_t minimum_first = occupied.begin()->first;
	std::size_t maximum_first = minimum_first;
	std::size_t minimum_second = occupied.begin()->second;
	std::size_t maximum_second = minimum_second;
	for (const auto &index : occupied) {
		minimum_first = std::min(minimum_first, index.first);
		maximum_first = std::max(maximum_first, index.first);
		minimum_second = std::min(minimum_second, index.second);
		maximum_second = std::max(maximum_second, index.second);
	}
	const double cell_area = voxel_edge_length_metres * voxel_edge_length_metres;
	const double envelope_cell_count =
		static_cast<double>(maximum_first - minimum_first + 1u) *
		static_cast<double>(maximum_second - minimum_second + 1u);
	const double occupied_cell_count = static_cast<double>(occupied.size());
	return ProjectedOccupancyMetrics{
		occupied_cell_count * cell_area,
		std::clamp(1.0 - occupied_cell_count / envelope_cell_count, 0.0, 1.0)};
}

}

VegetationCanopyOccupancyQualityReport
VegetationCanopyOccupancyQualityEvaluationService::evaluate(
	const VegetationCanopyOccupancyQualityEvidence &evidence,
	const VegetationCanopyOccupancyReconstructionPolicy &policy) const
{
	const double organ_label_fraction = evidence.totalPointCount() == 0u
		                                    ? 0.0
		                                    : static_cast<double>(
			                                      evidence.organLabeledPointCount()) /
			                                      evidence.totalPointCount();
	std::size_t recalled_holdout_count = 0u;
	for (const auto &holdout : evidence.holdoutFoliageCells()) {
		if (has_training_neighbor(
			    holdout, evidence.trainingOccupiedCells(),
			    policy.holdoutNeighborRadiusCells())) {
			++recalled_holdout_count;
		}
	}
	const double holdout_recall = evidence.holdoutFoliageCells().empty()
		                              ? 0.0
		                              : static_cast<double>(
			                                recalled_holdout_count) /
			                                evidence.holdoutFoliageCells().size();
	const auto xy = projected_metrics(
		evidence.reconstructedCells(), evidence.voxelEdgeLengthMetres(),
		[](const VegetationCanopyVoxelIndex &index) {
			return std::make_pair(index.xIndex(), index.yIndex());
		});
	const auto xz = projected_metrics(
		evidence.reconstructedCells(), evidence.voxelEdgeLengthMetres(),
		[](const VegetationCanopyVoxelIndex &index) {
			return std::make_pair(index.xIndex(), index.zIndex());
		});
	const auto yz = projected_metrics(
		evidence.reconstructedCells(), evidence.voxelEdgeLengthMetres(),
		[](const VegetationCanopyVoxelIndex &index) {
			return std::make_pair(index.yIndex(), index.zIndex());
		});
	std::vector<std::string> rejection_reasons;
	if (evidence.trainingOccupiedCells().empty()) {
		rejection_reasons.emplace_back(
			"Training split contains no occupied foliage cells.");
	}
	if (evidence.holdoutFoliageCells().empty()) {
		rejection_reasons.emplace_back(
			"Holdout split contains no foliage points.");
	}
	if (evidence.reconstructedCells().empty()) {
		rejection_reasons.emplace_back(
			"Reconstruction contains no occupied foliage cells.");
	}
	if (organ_label_fraction < policy.minimumOrganLabelFraction()) {
		rejection_reasons.emplace_back(
			"Organ label fraction is below the reconstruction quality boundary.");
	}
	if (holdout_recall < policy.minimumHoldoutRecall()) {
		rejection_reasons.emplace_back(
			"Holdout neighborhood recall is below the reconstruction quality boundary.");
	}
	return VegetationCanopyOccupancyQualityReport(
		evidence.reconstructionIdentifier(), evidence.datasetIdentifier(),
		evidence.sourcePayloadSha256(), evidence.trainingOccupiedCells().size(),
		evidence.holdoutFoliageCells().size(), organ_label_fraction,
		holdout_recall, xy.occupancy_area_square_metres,
		xz.occupancy_area_square_metres, yz.occupancy_area_square_metres,
		xy.gap_proxy, xz.gap_proxy, yz.gap_proxy,
		std::move(rejection_reasons));
}
