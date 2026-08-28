#pragma once

#include "vegetation/model/VegetationPointCloudBounds3d.h"
#include "vegetation/model/VegetationWoodyBranchAxis.h"
#include "vegetation/model/VegetationWoodyBranchConnection.h"
#include "vegetation/model/VegetationWoodyBranchGraphQualityReport.h"

#include <string>
#include <utility>
#include <vector>

enum class VegetationWoodyBranchGraphReconstructionScope
{
	CompleteForObservedWoodyEvidence
};

class VegetationWoodyBranchGraphReconstruction
{
public:
	VegetationWoodyBranchGraphReconstruction(
		std::string schema_version,
		std::string reconstruction_identifier,
		std::string reconstruction_job_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::string segmentation_identifier,
		std::string segmentation_algorithm_identifier,
		std::string root_axis_identifier,
		std::vector<VegetationWoodyBranchAxis> axes,
		std::vector<VegetationWoodyBranchConnection> connections,
		VegetationPointCloudBounds3d woody_evidence_bounds_metres,
		VegetationWoodyBranchGraphQualityReport quality_report)
		: schema_version_(std::move(schema_version)),
		  reconstruction_identifier_(std::move(reconstruction_identifier)),
		  reconstruction_job_identifier_(
			  std::move(reconstruction_job_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  segmentation_identifier_(std::move(segmentation_identifier)),
		  segmentation_algorithm_identifier_(
			  std::move(segmentation_algorithm_identifier)),
		  root_axis_identifier_(std::move(root_axis_identifier)),
		  axes_(std::move(axes)),
		  connections_(std::move(connections)),
		  woody_evidence_bounds_metres_(woody_evidence_bounds_metres),
		  quality_report_(std::move(quality_report))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &reconstructionJobIdentifier() const
	{
		return reconstruction_job_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	const std::string &segmentationIdentifier() const
	{
		return segmentation_identifier_;
	}
	const std::string &segmentationAlgorithmIdentifier() const
	{
		return segmentation_algorithm_identifier_;
	}
	const std::string &rootAxisIdentifier() const
	{
		return root_axis_identifier_;
	}
	VegetationWoodyBranchGraphReconstructionScope scope() const
	{
		return VegetationWoodyBranchGraphReconstructionScope::
			CompleteForObservedWoodyEvidence;
	}
	bool representsCompleteBiologicalTree() const { return false; }
	const std::vector<VegetationWoodyBranchAxis> &axes() const { return axes_; }
	const std::vector<VegetationWoodyBranchConnection> &connections() const
	{
		return connections_;
	}
	const VegetationPointCloudBounds3d &woodyEvidenceBoundsMetres() const
	{
		return woody_evidence_bounds_metres_;
	}
	const VegetationWoodyBranchGraphQualityReport &qualityReport() const
	{
		return quality_report_;
	}

private:
	std::string schema_version_;
	std::string reconstruction_identifier_;
	std::string reconstruction_job_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::string segmentation_identifier_;
	std::string segmentation_algorithm_identifier_;
	std::string root_axis_identifier_;
	std::vector<VegetationWoodyBranchAxis> axes_;
	std::vector<VegetationWoodyBranchConnection> connections_;
	VegetationPointCloudBounds3d woody_evidence_bounds_metres_{
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0),
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0)};
	VegetationWoodyBranchGraphQualityReport quality_report_;
};
