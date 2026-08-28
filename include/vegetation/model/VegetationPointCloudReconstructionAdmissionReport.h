#pragma once

#include "vegetation/model/VegetationPointCloudBounds3d.h"
#include "vegetation/model/VegetationPointCloudReconstructionTarget.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationPointCloudReconstructionAdmissionReport
{
public:
	VegetationPointCloudReconstructionAdmissionReport(
		std::string dataset_identifier,
		std::string source_payload_sha256,
		VegetationPointCloudReconstructionTarget target,
		std::size_t point_count,
		std::size_t duplicate_point_count,
		std::size_t woody_point_count,
		std::size_t foliage_point_count,
		std::size_t unknown_point_count,
		double duplicate_fraction,
		VegetationPointCloudBounds3d observed_bounds,
		std::vector<std::string> rejection_reasons)
		: dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  target_(target),
		  point_count_(point_count),
		  duplicate_point_count_(duplicate_point_count),
		  woody_point_count_(woody_point_count),
		  foliage_point_count_(foliage_point_count),
		  unknown_point_count_(unknown_point_count),
		  duplicate_fraction_(duplicate_fraction),
		  observed_bounds_(observed_bounds),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool admitted() const { return rejection_reasons_.empty(); }
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	VegetationPointCloudReconstructionTarget target() const { return target_; }
	std::size_t pointCount() const { return point_count_; }
	std::size_t duplicatePointCount() const { return duplicate_point_count_; }
	std::size_t woodyPointCount() const { return woody_point_count_; }
	std::size_t foliagePointCount() const { return foliage_point_count_; }
	std::size_t unknownPointCount() const { return unknown_point_count_; }
	double duplicateFraction() const { return duplicate_fraction_; }
	const VegetationPointCloudBounds3d &observedBounds() const
	{
		return observed_bounds_;
	}
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	VegetationPointCloudReconstructionTarget target_ =
		VegetationPointCloudReconstructionTarget::ShootArchitecture;
	std::size_t point_count_ = 0u;
	std::size_t duplicate_point_count_ = 0u;
	std::size_t woody_point_count_ = 0u;
	std::size_t foliage_point_count_ = 0u;
	std::size_t unknown_point_count_ = 0u;
	double duplicate_fraction_ = 0.0;
	VegetationPointCloudBounds3d observed_bounds_{
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0),
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0)};
	std::vector<std::string> rejection_reasons_;
};
