#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationCanopyOccupancyQualityReport
{
public:
	VegetationCanopyOccupancyQualityReport(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::size_t training_occupied_cell_count,
		std::size_t holdout_foliage_point_count,
		double organ_label_fraction,
		double holdout_neighborhood_recall,
		double xy_projected_occupancy_area_square_metres,
		double xz_projected_occupancy_area_square_metres,
		double yz_projected_occupancy_area_square_metres,
		double xy_projected_gap_proxy,
		double xz_projected_gap_proxy,
		double yz_projected_gap_proxy,
		std::vector<std::string> rejection_reasons)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  training_occupied_cell_count_(training_occupied_cell_count),
		  holdout_foliage_point_count_(holdout_foliage_point_count),
		  organ_label_fraction_(organ_label_fraction),
		  holdout_neighborhood_recall_(holdout_neighborhood_recall),
		  xy_projected_occupancy_area_square_metres_(
			  xy_projected_occupancy_area_square_metres),
		  xz_projected_occupancy_area_square_metres_(
			  xz_projected_occupancy_area_square_metres),
		  yz_projected_occupancy_area_square_metres_(
			  yz_projected_occupancy_area_square_metres),
		  xy_projected_gap_proxy_(xy_projected_gap_proxy),
		  xz_projected_gap_proxy_(xz_projected_gap_proxy),
		  yz_projected_gap_proxy_(yz_projected_gap_proxy),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool acceptedForGeometryUse() const { return rejection_reasons_.empty(); }
	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	std::size_t trainingOccupiedCellCount() const
	{
		return training_occupied_cell_count_;
	}
	std::size_t holdoutFoliagePointCount() const
	{
		return holdout_foliage_point_count_;
	}
	double organLabelFraction() const { return organ_label_fraction_; }
	double holdoutNeighborhoodRecall() const
	{
		return holdout_neighborhood_recall_;
	}
	double xyProjectedOccupancyAreaSquareMetres() const
	{
		return xy_projected_occupancy_area_square_metres_;
	}
	double xzProjectedOccupancyAreaSquareMetres() const
	{
		return xz_projected_occupancy_area_square_metres_;
	}
	double yzProjectedOccupancyAreaSquareMetres() const
	{
		return yz_projected_occupancy_area_square_metres_;
	}
	double xyProjectedGapProxy() const { return xy_projected_gap_proxy_; }
	double xzProjectedGapProxy() const { return xz_projected_gap_proxy_; }
	double yzProjectedGapProxy() const { return yz_projected_gap_proxy_; }
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::size_t training_occupied_cell_count_ = 0u;
	std::size_t holdout_foliage_point_count_ = 0u;
	double organ_label_fraction_ = 0.0;
	double holdout_neighborhood_recall_ = 0.0;
	double xy_projected_occupancy_area_square_metres_ = 0.0;
	double xz_projected_occupancy_area_square_metres_ = 0.0;
	double yz_projected_occupancy_area_square_metres_ = 0.0;
	double xy_projected_gap_proxy_ = 0.0;
	double xz_projected_gap_proxy_ = 0.0;
	double yz_projected_gap_proxy_ = 0.0;
	std::vector<std::string> rejection_reasons_;
};
