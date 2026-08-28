#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/model/VegetationWoodyAxisCylinder.h"
#include "vegetation/model/VegetationWoodyAxisRadiusStation.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyAxisQualityEvidence
{
public:
	VegetationWoodyAxisQualityEvidence(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::size_t total_point_count,
		std::size_t woody_point_count,
		std::size_t initial_training_woody_point_count,
		std::size_t excluded_training_outlier_count,
		std::vector<VegetationMeasuredPoint3d> training_points_metres,
		std::vector<VegetationMeasuredPoint3d> holdout_points_metres,
		VegetationMeasuredPoint3d axis_centroid_metres,
		VegetationMeasuredPoint3d principal_axis_direction,
		double minor_variance,
		double intermediate_variance,
		double principal_variance,
		std::vector<VegetationWoodyAxisRadiusStation> radius_stations,
		std::vector<VegetationWoodyAxisCylinder> cylinders)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  total_point_count_(total_point_count),
		  woody_point_count_(woody_point_count),
		  initial_training_woody_point_count_(
			  initial_training_woody_point_count),
		  excluded_training_outlier_count_(
			  excluded_training_outlier_count),
		  training_points_metres_(std::move(training_points_metres)),
		  holdout_points_metres_(std::move(holdout_points_metres)),
		  axis_centroid_metres_(axis_centroid_metres),
		  principal_axis_direction_(principal_axis_direction),
		  minor_variance_(minor_variance),
		  intermediate_variance_(intermediate_variance),
		  principal_variance_(principal_variance),
		  radius_stations_(std::move(radius_stations)),
		  cylinders_(std::move(cylinders))
	{
	}

	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	std::size_t totalPointCount() const { return total_point_count_; }
	std::size_t woodyPointCount() const { return woody_point_count_; }
	std::size_t initialTrainingWoodyPointCount() const
	{
		return initial_training_woody_point_count_;
	}
	std::size_t excludedTrainingOutlierCount() const
	{
		return excluded_training_outlier_count_;
	}
	const std::vector<VegetationMeasuredPoint3d> &trainingPointsMetres() const
	{
		return training_points_metres_;
	}
	const std::vector<VegetationMeasuredPoint3d> &holdoutPointsMetres() const
	{
		return holdout_points_metres_;
	}
	const VegetationMeasuredPoint3d &axisCentroidMetres() const
	{
		return axis_centroid_metres_;
	}
	const VegetationMeasuredPoint3d &principalAxisDirection() const
	{
		return principal_axis_direction_;
	}
	double minorVariance() const { return minor_variance_; }
	double intermediateVariance() const { return intermediate_variance_; }
	double principalVariance() const { return principal_variance_; }
	const std::vector<VegetationWoodyAxisRadiusStation> &radiusStations() const
	{
		return radius_stations_;
	}
	const std::vector<VegetationWoodyAxisCylinder> &cylinders() const
	{
		return cylinders_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::size_t total_point_count_ = 0u;
	std::size_t woody_point_count_ = 0u;
	std::size_t initial_training_woody_point_count_ = 0u;
	std::size_t excluded_training_outlier_count_ = 0u;
	std::vector<VegetationMeasuredPoint3d> training_points_metres_;
	std::vector<VegetationMeasuredPoint3d> holdout_points_metres_;
	VegetationMeasuredPoint3d axis_centroid_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d principal_axis_direction_{0.0, 0.0, 1.0};
	double minor_variance_ = 0.0;
	double intermediate_variance_ = 0.0;
	double principal_variance_ = 0.0;
	std::vector<VegetationWoodyAxisRadiusStation> radius_stations_;
	std::vector<VegetationWoodyAxisCylinder> cylinders_;
};
