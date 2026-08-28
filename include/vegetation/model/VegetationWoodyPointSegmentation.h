#pragma once

#include "vegetation/model/VegetationWoodyAxisSegmentDefinition.h"
#include "vegetation/model/VegetationWoodyPointSegmentAssignment.h"

#include <string>
#include <utility>
#include <vector>

class VegetationWoodyPointSegmentation
{
public:
	VegetationWoodyPointSegmentation(
		std::string schema_version,
		std::string segmentation_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::string segmentation_algorithm_identifier,
		std::string root_axis_identifier,
		bool complete_for_observed_woody_points,
		std::vector<VegetationWoodyAxisSegmentDefinition> axis_definitions,
		std::vector<VegetationWoodyPointSegmentAssignment> point_assignments)
		: schema_version_(std::move(schema_version)),
		  segmentation_identifier_(std::move(segmentation_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  segmentation_algorithm_identifier_(
			  std::move(segmentation_algorithm_identifier)),
		  root_axis_identifier_(std::move(root_axis_identifier)),
		  complete_for_observed_woody_points_(
			  complete_for_observed_woody_points),
		  axis_definitions_(std::move(axis_definitions)),
		  point_assignments_(std::move(point_assignments))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &segmentationIdentifier() const
	{
		return segmentation_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	const std::string &segmentationAlgorithmIdentifier() const
	{
		return segmentation_algorithm_identifier_;
	}
	const std::string &rootAxisIdentifier() const
	{
		return root_axis_identifier_;
	}
	bool completeForObservedWoodyPoints() const
	{
		return complete_for_observed_woody_points_;
	}
	const std::vector<VegetationWoodyAxisSegmentDefinition> &axisDefinitions()
		const
	{
		return axis_definitions_;
	}
	const std::vector<VegetationWoodyPointSegmentAssignment> &pointAssignments()
		const
	{
		return point_assignments_;
	}

private:
	std::string schema_version_;
	std::string segmentation_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::string segmentation_algorithm_identifier_;
	std::string root_axis_identifier_;
	bool complete_for_observed_woody_points_ = false;
	std::vector<VegetationWoodyAxisSegmentDefinition> axis_definitions_;
	std::vector<VegetationWoodyPointSegmentAssignment> point_assignments_;
};
