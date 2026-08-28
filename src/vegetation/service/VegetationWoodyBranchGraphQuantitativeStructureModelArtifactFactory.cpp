#include "vegetation/service/VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory.h"

#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"

#include <json/json.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>

namespace {

Json::Value point_json(const VegetationMeasuredPoint3d &point)
{
	Json::Value value(Json::arrayValue);
	value.append(point.x());
	value.append(point.y());
	value.append(point.z());
	return value;
}

std::string compact_json(const Json::Value &document)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	writer["commentStyle"] = "None";
	writer["emitUTF8"] = true;
	return Json::writeString(writer, document);
}

}

std::optional<VegetationMeasuredSourceArtifact>
VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory::create(
	const VegetationWoodyBranchGraphReconstruction &reconstruction) const
{
	if (!reconstruction.qualityReport().acceptedForObservedWoodyGraph() ||
	    reconstruction.axes().size() < 2u ||
	    reconstruction.qualityReport().maximumBranchOrder() == 0u) {
		return std::nullopt;
	}
	Json::Value document(Json::objectValue);
	document["schema_version"] = "ProGen3D-QuantitativeStructureModel-v1";
	document["plant_architecture"] = "Tree";
	document["graph_identifier"] = reconstruction.reconstructionIdentifier();
	document["length_unit"] = "m";
	document["derived_from_dataset_identifier"] =
		reconstruction.datasetIdentifier();
	document["derived_from_source_payload_sha256"] =
		reconstruction.sourcePayloadSha256();
	document["segmentation_identifier"] =
		reconstruction.segmentationIdentifier();
	document["segmentation_algorithm_identifier"] =
		reconstruction.segmentationAlgorithmIdentifier();
	document["reconstruction_scope"] = "CompleteForObservedWoodyEvidence";
	document["complete_biological_tree"] = false;

	Json::Value quality(Json::objectValue);
	quality["assignment_coverage_fraction"] =
		reconstruction.qualityReport().assignmentCoverageFraction();
	quality["minimum_assignment_confidence"] =
		reconstruction.qualityReport().minimumAssignmentConfidence();
	quality["maximum_holdout_surface_rmse_metres"] =
		reconstruction.qualityReport().maximumHoldoutSurfaceRmseMetres();
	quality["minimum_holdout_surface_coverage_fraction"] =
		reconstruction.qualityReport()
			.minimumHoldoutSurfaceCoverageFraction();
	quality["maximum_attachment_surface_gap_metres"] =
		reconstruction.qualityReport().maximumAttachmentSurfaceGapMetres();
	document["reconstruction_quality"] = quality;

	Json::Value cylinders(Json::arrayValue);
	for (const auto &axis : reconstruction.axes()) {
		for (const auto &cylinder : axis.cylinders()) {
			Json::Value value(Json::objectValue);
			value["identifier"] = cylinder.cylinderIdentifier();
			value["parent_identifier"] =
				cylinder.parentCylinderIdentifier();
			value["branch_order"] = static_cast<Json::UInt64>(
				cylinder.branchOrder());
			value["start"] = point_json(cylinder.startPointMetres());
			value["end"] = point_json(cylinder.endPointMetres());
			value["radius"] = cylinder.radiusMetres();
			cylinders.append(value);
		}
	}
	document["cylinders"] = cylinders;
	std::ostringstream uncertainty;
	uncertainty
		<< "Derived from explicit point-to-axis segmentation and exact source "
		<< reconstruction.sourcePayloadSha256()
		<< "; complete only for observed woody points; maximum holdout surface "
		   "RMSE="
		<< reconstruction.qualityReport().maximumHoldoutSurfaceRmseMetres()
		<< " m; maximum attachment surface gap="
		<< reconstruction.qualityReport().maximumAttachmentSurfaceGapMetres()
		<< " m.";
	const VegetationMeasuredSourceArtifact unsigned_artifact(
		"ProGen3D-VegetationMeasuredSourceArtifact-v1",
		"derived-qsm:" + reconstruction.reconstructionIdentifier(),
		"ProGen3D quality-admitted segmented woody branch graph reconstruction",
		"derived://point-cloud-sha256/" + reconstruction.sourcePayloadSha256(),
		"application/vnd.progen3d.qsm+json", "LocalPlantXYZ-ZUp", "SI",
		"ProGen3D-SegmentedWoodyBranchGraph-v1", uncertainty.str(),
		compact_json(document), std::string());
	return VegetationMeasuredSourceArtifactHashService().attachPayloadSha256(
		unsigned_artifact);
}
