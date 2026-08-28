#pragma once

#include "vegetation/model/VegetationPointCloudBounds3d.h"
#include "vegetation/model/VegetationWoodyAxisCylinder.h"
#include "vegetation/model/VegetationWoodyAxisQualityReport.h"
#include "vegetation/model/VegetationWoodyAxisRadiusStation.h"

#include <string>
#include <utility>
#include <vector>

enum class VegetationWoodyAxisReconstructionScope
{
	PrimaryAxisOnly
};

class VegetationWoodyAxisReconstruction
{
public:
	VegetationWoodyAxisReconstruction(
		std::string schema_version,
		std::string reconstruction_identifier,
		std::string reconstruction_job_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		VegetationMeasuredPoint3d axis_centroid_metres,
		VegetationMeasuredPoint3d principal_axis_direction,
		std::vector<VegetationWoodyAxisRadiusStation> radius_stations,
		std::vector<VegetationWoodyAxisCylinder> cylinders,
		VegetationPointCloudBounds3d woody_evidence_bounds_metres,
		VegetationWoodyAxisQualityReport quality_report)
		: schema_version_(std::move(schema_version)),
		  reconstruction_identifier_(std::move(reconstruction_identifier)),
		  reconstruction_job_identifier_(
			  std::move(reconstruction_job_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  axis_centroid_metres_(axis_centroid_metres),
		  principal_axis_direction_(principal_axis_direction),
		  radius_stations_(std::move(radius_stations)),
		  cylinders_(std::move(cylinders)),
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
	VegetationWoodyAxisReconstructionScope scope() const
	{
		return VegetationWoodyAxisReconstructionScope::PrimaryAxisOnly;
	}
	bool representsCompleteBranchTopology() const { return false; }
	const VegetationMeasuredPoint3d &axisCentroidMetres() const
	{
		return axis_centroid_metres_;
	}
	const VegetationMeasuredPoint3d &principalAxisDirection() const
	{
		return principal_axis_direction_;
	}
	const std::vector<VegetationWoodyAxisRadiusStation> &radiusStations() const
	{
		return radius_stations_;
	}
	const std::vector<VegetationWoodyAxisCylinder> &cylinders() const
	{
		return cylinders_;
	}
	const VegetationPointCloudBounds3d &woodyEvidenceBoundsMetres() const
	{
		return woody_evidence_bounds_metres_;
	}
	const VegetationWoodyAxisQualityReport &qualityReport() const
	{
		return quality_report_;
	}

private:
	std::string schema_version_;
	std::string reconstruction_identifier_;
	std::string reconstruction_job_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	VegetationMeasuredPoint3d axis_centroid_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d principal_axis_direction_{0.0, 0.0, 1.0};
	std::vector<VegetationWoodyAxisRadiusStation> radius_stations_;
	std::vector<VegetationWoodyAxisCylinder> cylinders_;
	VegetationPointCloudBounds3d woody_evidence_bounds_metres_{
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0),
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0)};
	VegetationWoodyAxisQualityReport quality_report_;
};
