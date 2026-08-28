#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationPointCloudReconstructionAdmissionPolicy.h"
#include "vegetation/service/VegetationAutomatedWoodySegmentationEnsembleService.h"
#include "vegetation/service/VegetationCanopyOccupancyReconstructionService.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"
#include "vegetation/service/VegetationPrimaryWoodyAxisReconstructionService.h"
#include "vegetation/service/VegetationPointCloudFormatCapabilityCatalog.h"
#include "vegetation/service/VegetationPointCloudIngestionService.h"
#include "vegetation/service/VegetationPointCloudReconstructionAdmissionService.h"
#include "vegetation/service/VegetationPointCloudReconstructionJobFactory.h"
#include "vegetation/service/VegetationQuantitativeStructureModelAdapter.h"
#include "vegetation/service/VegetationSegmentedWoodyBranchGraphReconstructionService.h"
#include "vegetation/service/VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory.h"
#include "vegetation/service/VegetationWoodyCoverSetSegmentationService.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using IssueCode = VegetationPointCloudIngestionIssueCode;
using ObservationKind = VegetationPointCloudIngestionObservationKind;
using ReconstructionIssueCode =
	VegetationCanopyOccupancyReconstructionIssueCode;
using WoodyAxisIssueCode = VegetationWoodyAxisReconstructionIssueCode;
using WoodyGraphIssueCode =
	VegetationWoodyBranchGraphReconstructionIssueCode;
using AutomatedSegmentationIssueCode =
	VegetationAutomatedWoodySegmentationEnsembleIssueCode;
using SegmentationCandidateIssueCode =
	VegetationWoodySegmentationCandidateIssueCode;

std::string read_text_file(const std::string &path)
{
	std::ifstream stream(path, std::ios::binary);
	assert(stream.good());
	return std::string(
		std::istreambuf_iterator<char>(stream),
		std::istreambuf_iterator<char>());
}

std::string read_hex_file(const std::string &path)
{
	const std::string text = read_text_file(path);
	std::string digits;
	for (const unsigned char character : text) {
		if (!std::isspace(character)) digits.push_back(character);
	}
	assert(digits.size() % 2u == 0u);
	std::string payload;
	payload.reserve(digits.size() / 2u);
	for (std::size_t index = 0u; index < digits.size(); index += 2u) {
		const auto nibble = [](char character) -> std::uint8_t {
			if (character >= '0' && character <= '9') {
				return static_cast<std::uint8_t>(character - '0');
			}
			if (character >= 'a' && character <= 'f') {
				return static_cast<std::uint8_t>(character - 'a' + 10);
			}
			if (character >= 'A' && character <= 'F') {
				return static_cast<std::uint8_t>(character - 'A' + 10);
			}
			assert(false);
			return 0u;
		};
		payload.push_back(static_cast<char>(
			(nibble(digits[index]) << 4u) | nibble(digits[index + 1u])));
	}
	return payload;
}

VegetationMeasuredSourceArtifact source_artifact(
	const std::string &source_identifier,
	const std::string &media_type,
	std::string payload)
{
	const VegetationMeasuredSourceArtifact unsigned_artifact(
		"ProGen3D-VegetationMeasuredSourceArtifact-v1", source_identifier,
		"Frozen deterministic D3A point-cloud fixture",
		"fixture://vegetation-native-sources/" + source_identifier, media_type,
		"LocalPlantXYZ-ZUp", "SI", "Frozen point-cloud fixture",
		"Fixture coordinates and attributes retain their exact source hash.",
		std::move(payload), std::string());
	return VegetationMeasuredSourceArtifactHashService().attachPayloadSha256(
		unsigned_artifact);
}

VegetationPointCloudIngestionContext context(
	const std::string &schema,
	const std::string &dataset_identifier,
	std::size_t maximum_points = 1000u,
	bool reject_duplicates = true)
{
	return VegetationPointCloudIngestionContext(
		schema, dataset_identifier, "m",
		VegetationPointCloudIngestionPolicy(
			2u * 1024u * 1024u, maximum_points, reject_duplicates));
}

bool has_issue(
	const VegetationPointCloudIngestionReport &report,
	IssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](const VegetationPointCloudIngestionIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_observation(
	const VegetationPointCloudIngestionReport &report,
	ObservationKind expected_kind)
{
	return std::any_of(
		report.observations().begin(), report.observations().end(),
		[expected_kind](
			const VegetationPointCloudIngestionObservation &observation) {
			return observation.kind() == expected_kind;
		});
}

bool has_reconstruction_issue(
	const VegetationCanopyOccupancyReconstructionReport &report,
	ReconstructionIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](
			const VegetationCanopyOccupancyReconstructionIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_woody_axis_issue(
	const VegetationWoodyAxisReconstructionReport &report,
	WoodyAxisIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](const VegetationWoodyAxisReconstructionIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_woody_graph_issue(
	const VegetationWoodyBranchGraphReconstructionReport &report,
	WoodyGraphIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](
			const VegetationWoodyBranchGraphReconstructionIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_automated_segmentation_issue(
	const VegetationAutomatedWoodySegmentationEnsembleReport &report,
	AutomatedSegmentationIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](
			const VegetationAutomatedWoodySegmentationEnsembleIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_segmentation_candidate_issue(
	const VegetationWoodySegmentationCandidateReport &report,
	SegmentationCandidateIssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](const VegetationWoodySegmentationCandidateIssue &issue) {
			return issue.code() == expected_code;
		});
}

VegetationWoodyAxisReconstructionPolicy woody_axis_policy(
	double minimum_principal_variance_fraction = 0.95,
	std::size_t maximum_cylinder_count = 8u,
	std::size_t minimum_woody_point_count = 32u)
{
	return VegetationWoodyAxisReconstructionPolicy(
		0.75, minimum_woody_point_count, 24u, 5u,
		maximum_cylinder_count, 4u, 4.0, 0.10, 0.90,
		minimum_principal_variance_fraction, 0.001, 0.001, 1.0, 0.0,
		0.10);
}

VegetationWoodyBranchGraphReconstructionPolicy woody_graph_policy(
	double maximum_attachment_surface_gap_metres = 0.10,
	std::size_t maximum_total_cylinder_count = 16u,
	double minimum_assignment_confidence = 0.95)
{
	return VegetationWoodyBranchGraphReconstructionPolicy(
		16u, 12u, 5u, 8u, maximum_total_cylinder_count, 4u,
		minimum_assignment_confidence, 0.0, 0.95, 0.001, 0.001,
		0.001, 1.0, maximum_attachment_surface_gap_metres, 0.0, 0.02,
		0.30);
}

VegetationWoodyBranchGraphReconstructionPolicy automated_graph_policy()
{
	return VegetationWoodyBranchGraphReconstructionPolicy(
		12u, 8u, 2u, 8u, 20u, 4u, 0.60, 0.0, 0.80, 0.06, 0.06,
		0.08, 0.70, 0.15, 0.50, 0.015, 0.25);
}

std::vector<VegetationWoodySegmentationParameterSet>
automated_segmentation_parameter_sets()
{
	const auto parameter_set = [](const std::string &identifier,
	                              double cover_cell_size_metres) {
		return VegetationWoodySegmentationParameterSet(
			identifier, cover_cell_size_metres, 1.85, 1u, 3u, 64u, 8u,
			0.55, 0.60, 2u, 0.35);
	};
	return {
		parameter_set("cover-062", 0.62),
		parameter_set("cover-065", 0.65),
		parameter_set("cover-080", 0.80),
	};
}

VegetationWoodySegmentationSensitivityPolicy automated_sensitivity_policy(
	double minimum_assignment_agreement_fraction = 0.88)
{
	return VegetationWoodySegmentationSensitivityPolicy(
		3u, 2u, 0.66, minimum_assignment_agreement_fraction, 1.0, 1.0,
		0.30, 0.20, 0.35);
}

VegetationWoodyPointSegmentation segmented_tree_assignments(
	const VegetationPointCloudDataset &dataset,
	double assignment_confidence = 0.99,
	bool complete_for_observed_woody_points = true,
	bool omit_last_assignment = false)
{
	std::vector<VegetationWoodyPointSegmentAssignment> assignments;
	for (const auto &point : dataset.points()) {
		assert(point.classification().has_value());
		std::string axis_identifier;
		switch (*point.classification()) {
		case 10u: axis_identifier = "axis:trunk"; break;
		case 11u: axis_identifier = "axis:branch-primary"; break;
		case 12u: axis_identifier = "axis:branch-secondary"; break;
		default: assert(false); break;
		}
		assignments.emplace_back(
			point.sourceIndex(), axis_identifier, assignment_confidence);
	}
	if (omit_last_assignment) assignments.pop_back();
	return VegetationWoodyPointSegmentation(
		"ProGen3D-VegetationWoodyPointSegmentation-v1",
		"segmentation:three-axis-tree", dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(),
		"Fixture-Segmentation-ClassificationMap-v1", "axis:trunk",
		complete_for_observed_woody_points,
		{
			VegetationWoodyAxisSegmentDefinition(
				"axis:trunk", std::string(), 0u, 0.75),
			VegetationWoodyAxisSegmentDefinition(
				"axis:branch-primary", "axis:trunk", 1u, 0.50),
			VegetationWoodyAxisSegmentDefinition(
				"axis:branch-secondary", "axis:branch-primary", 2u, 0.40),
		},
		std::move(assignments));
}

void verify_capability_catalog()
{
	const VegetationPointCloudFormatCapabilityCatalog catalog;
	assert(catalog.capabilities().size() == 11u);
	const auto ply_ascii = catalog.findCapability(
		"application/ply", "PLY-1.0", VegetationPointCloudEncoding::Ascii);
	assert(ply_ascii.has_value());
	assert(
		ply_ascii->capabilityLevel() ==
		VegetationPointCloudCapabilityLevel::PointRecords);
	const auto ply_binary = catalog.findCapability(
		"application/ply", "PLY-1.0",
		VegetationPointCloudEncoding::BinaryLittleEndian);
	assert(ply_binary.has_value());
	assert(
		ply_binary->capabilityLevel() ==
		VegetationPointCloudCapabilityLevel::PointRecords);
	const auto las_15 = catalog.findCapability(
		"application/vnd.las", "LAS-1.5",
		VegetationPointCloudEncoding::BinaryLittleEndian);
	assert(las_15.has_value());
	assert(
		las_15->capabilityLevel() ==
		VegetationPointCloudCapabilityLevel::Unsupported);
	const auto laz = catalog.findCapability(
		"application/vnd.laz", "LAZ-1.4",
		VegetationPointCloudEncoding::Compressed);
	assert(laz.has_value());
	assert(laz->requiredDependency() == "LASzip or PDAL");
	assert(!catalog.findCapability(
		"application/ply", "PLY-2.0", VegetationPointCloudEncoding::Ascii)
			.has_value());
}

VegetationPointCloudIngestionReport verify_ply_decode(
	const VegetationMeasuredSourceArtifact &ply_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		ply_artifact, context("PLY-1.0", "dataset:ply-canopy"));
	assert(decoded.pointsSucceeded());
	assert(decoded.decoderIdentifier() == "VegetationPlyPointCloudDecoder");
	assert(decoded.sourceMetadata().has_value());
	assert(decoded.dataset().has_value());
	assert(decoded.sourceMetadata()->declaredPointCount() == 12u);
	assert(decoded.sourceMetadata()->sourcePayloadSha256() ==
	       ply_artifact.payloadSha256());
	assert(!decoded.sourceMetadata()->declaredBoundsMetres().has_value());
	assert(decoded.dataset()->points().size() == 12u);
	assert(decoded.dataset()->points()[0].organClass() ==
	       VegetationPointCloudOrganClass::Woody);
	assert(decoded.dataset()->points()[4].organClass() ==
	       VegetationPointCloudOrganClass::Foliage);
	assert(decoded.dataset()->points()[0].red().has_value());
	assert(*decoded.dataset()->points()[0].red() == 18000u);
	const auto &bounds = decoded.dataset()->observedBounds();
	assert(std::abs(bounds.minimum().x() + 1.0) < 1.0e-12);
	assert(std::abs(bounds.minimum().y() + 1.0) < 1.0e-12);
	assert(std::abs(bounds.minimum().z()) < 1.0e-12);
	assert(std::abs(bounds.maximum().x() - 1.0) < 1.0e-12);
	assert(std::abs(bounds.maximum().y() - 1.0) < 1.0e-12);
	assert(std::abs(bounds.maximum().z() - 3.0) < 1.0e-12);
	assert(has_observation(decoded, ObservationKind::AcceptedPoint));
	assert(has_observation(decoded, ObservationKind::DeferredProperty));
	return decoded;
}

VegetationPointCloudIngestionReport verify_las_decode(
	const VegetationMeasuredSourceArtifact &las_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		las_artifact, context("LAS-1.4", "dataset:las-canopy"));
	assert(decoded.pointsSucceeded());
	assert(decoded.decoderIdentifier() == "VegetationLasPointCloudDecoder");
	assert(decoded.sourceMetadata().has_value());
	assert(decoded.dataset().has_value());
	assert(decoded.sourceMetadata()->declaredPointCount() == 4u);
	assert(decoded.sourceMetadata()->pointRecordLengthBytes() == 30u);
	assert(decoded.sourceMetadata()->sourceFormatVersion() == "1.4");
	assert(decoded.sourceMetadata()->sourcePayloadSha256() ==
	       las_artifact.payloadSha256());
	assert(decoded.sourceMetadata()->declaredBoundsMetres().has_value());
	assert(decoded.dataset()->points().size() == 4u);
	assert(*decoded.dataset()->points()[1].returnNumber() == 2u);
	assert(*decoded.dataset()->points()[2].classification() == 5u);
	assert(decoded.dataset()->points()[0].organClass() ==
	       VegetationPointCloudOrganClass::Unknown);
	const auto &last = decoded.dataset()->points()[3].positionMetres();
	assert(std::abs(last.x() - 1.0) < 1.0e-12);
	assert(std::abs(last.y() - 2.0) < 1.0e-12);
	assert(std::abs(last.z() - 3.0) < 1.0e-12);
	assert(has_observation(decoded, ObservationKind::AcceptedPoint));
	assert(has_observation(decoded, ObservationKind::DeferredProperty));
	return decoded;
}

VegetationPointCloudIngestionReport verify_binary_ply_decode(
	const VegetationMeasuredSourceArtifact &artifact,
	VegetationPointCloudEncoding expected_encoding,
	const std::string &dataset_identifier)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		artifact, context("PLY-1.0", dataset_identifier));
	assert(decoded.pointsSucceeded());
	assert(decoded.sourceMetadata().has_value());
	assert(decoded.dataset().has_value());
	assert(decoded.sourceMetadata()->encoding() == expected_encoding);
	assert(decoded.sourceMetadata()->declaredPointCount() == 4u);
	assert(decoded.sourceMetadata()->pointRecordLengthBytes() == 42u);
	assert(decoded.sourceMetadata()->sourcePayloadSha256() ==
	       artifact.payloadSha256());
	assert(decoded.dataset()->points().size() == 4u);
	assert(decoded.dataset()->points()[0].organClass() ==
	       VegetationPointCloudOrganClass::Woody);
	assert(decoded.dataset()->points()[3].organClass() ==
	       VegetationPointCloudOrganClass::Foliage);
	assert(*decoded.dataset()->points()[1].returnNumber() == 2u);
	assert(*decoded.dataset()->points()[3].red() == 14000u);
	const auto &last = decoded.dataset()->points()[3].positionMetres();
	assert(std::abs(last.x() - 1.0) < 1.0e-12);
	assert(std::abs(last.y() - 2.0) < 1.0e-12);
	assert(std::abs(last.z() - 3.0) < 1.0e-12);
	return decoded;
}

void verify_reconstruction_admission(
	const VegetationPointCloudIngestionReport &ply,
	const VegetationPointCloudIngestionReport &las)
{
	const VegetationPointCloudReconstructionAdmissionService admission;
	const auto canopy_report = admission.evaluate(
		*ply.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::CanopyOptics, 10u, 0.5,
			0.0, true));
	assert(canopy_report.admitted());
	assert(canopy_report.woodyPointCount() == 4u);
	assert(canopy_report.foliagePointCount() == 8u);
	assert(canopy_report.unknownPointCount() == 0u);
	assert(canopy_report.duplicatePointCount() == 0u);
	assert(canopy_report.sourcePayloadSha256() ==
	       ply.dataset()->sourceMetadata().sourcePayloadSha256());
	const VegetationPointCloudReconstructionJobFactory jobs;
	const auto canopy_job = jobs.create(
		"job:ply-canopy-001", *ply.dataset(), canopy_report,
		"ProGen3D-CanopyVoxelOccupancy-v1", 1000u, 42u);
	assert(canopy_job.has_value());
	const VegetationCanopyOccupancyReconstructionPolicy occupancy_policy(
		1.0, 1u, 100u, 3u, 1u, 0.9, 1.0);
	const VegetationCanopyOccupancyReconstructionService reconstruction;
	const auto occupancy = reconstruction.reconstruct(
		"canopy-occupancy:ply-001", *ply.dataset(), canopy_report,
		*canopy_job, occupancy_policy);
	assert(occupancy.succeeded());
	assert(occupancy.qualityReport().has_value());
	assert(occupancy.reconstruction().has_value());
	assert(occupancy.reconstruction()->schemaVersion() ==
	       "ProGen3D-VegetationCanopyOccupancyReconstruction-v1");
	assert(occupancy.reconstruction()->cells().size() == 7u);
	assert(occupancy.qualityReport()->trainingOccupiedCellCount() == 5u);
	assert(occupancy.qualityReport()->holdoutFoliagePointCount() == 2u);
	assert(std::abs(occupancy.qualityReport()->organLabelFraction() - 1.0) <
	       1.0e-12);
	assert(std::abs(
		       occupancy.qualityReport()->holdoutNeighborhoodRecall() - 1.0) <
	       1.0e-12);
	assert(std::abs(
		       occupancy.qualityReport()
			       ->xyProjectedOccupancyAreaSquareMetres() -
		       6.0) < 1.0e-12);
	assert(std::abs(
		       occupancy.qualityReport()->xyProjectedGapProxy() - 1.0 / 3.0) <
	       1.0e-12);
	assert(occupancy.reconstruction()->sourcePayloadSha256() ==
	       ply.dataset()->sourceMetadata().sourcePayloadSha256());

	const auto repeated_occupancy = reconstruction.reconstruct(
		"canopy-occupancy:ply-001", *ply.dataset(), canopy_report,
		*canopy_job, occupancy_policy);
	assert(repeated_occupancy.succeeded());
	assert(repeated_occupancy.reconstruction()->cells().size() ==
	       occupancy.reconstruction()->cells().size());
	for (std::size_t index = 0u;
	     index < occupancy.reconstruction()->cells().size(); ++index) {
		const auto &first = occupancy.reconstruction()->cells()[index];
		const auto &second = repeated_occupancy.reconstruction()->cells()[index];
		assert(first.index() == second.index());
		assert(first.foliagePointCount() == second.foliagePointCount());
	}

	const auto rejected_quality = reconstruction.reconstruct(
		"canopy-occupancy:ply-low-recall", *ply.dataset(), canopy_report,
		*canopy_job,
		VegetationCanopyOccupancyReconstructionPolicy(
			0.1, 1u, 1000u, 3u, 0u, 0.5, 1.0));
	assert(!rejected_quality.succeeded());
	assert(rejected_quality.qualityReport().has_value());
	assert(has_reconstruction_issue(
		rejected_quality, ReconstructionIssueCode::QualityGateRejected));

	const auto cell_limited = reconstruction.reconstruct(
		"canopy-occupancy:ply-cell-limit", *ply.dataset(), canopy_report,
		*canopy_job,
		VegetationCanopyOccupancyReconstructionPolicy(
			1.0, 1u, 3u, 3u, 1u, 0.9, 1.0));
	assert(!cell_limited.succeeded());
	assert(has_reconstruction_issue(
		cell_limited, ReconstructionIssueCode::OccupiedCellLimitExceeded));

	const auto unsupported_job = jobs.create(
		"job:ply-canopy-unsupported", *ply.dataset(), canopy_report,
		"Unsupported-Canopy-Algorithm", 1000u, 42u);
	assert(unsupported_job.has_value());
	const auto unsupported_algorithm = reconstruction.reconstruct(
		"canopy-occupancy:unsupported", *ply.dataset(), canopy_report,
		*unsupported_job, occupancy_policy);
	assert(!unsupported_algorithm.succeeded());
	assert(has_reconstruction_issue(
		unsupported_algorithm, ReconstructionIssueCode::UnsupportedAlgorithm));

	const auto las_canopy_report = admission.evaluate(
		*las.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::CanopyOptics, 4u, 0.5,
			0.0, true));
	assert(!las_canopy_report.admitted());
	assert(!las_canopy_report.rejectionReasons().empty());

	const auto shoot_report = admission.evaluate(
		*las.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::ShootArchitecture, 4u,
			0.5, 0.0, false));
	assert(shoot_report.admitted());
	const auto job = jobs.create(
		"job:las-shoot-001", *las.dataset(), shoot_report,
		"TreeQSM-compatible-reconstruction-proposal-v1", 10000u, 42u);
	assert(job.has_value());
	assert(job->datasetIdentifier() == "dataset:las-canopy");
	assert(job->target() ==
	       VegetationPointCloudReconstructionTarget::ShootArchitecture);
	assert(job->deterministicSeed() == 42u);
	assert(!jobs.create(
		"job:mismatched", *ply.dataset(), shoot_report,
		"TreeQSM-compatible-reconstruction-proposal-v1", 10000u, 42u)
			.has_value());
}

void verify_primary_woody_axis_reconstruction(
	const VegetationMeasuredSourceArtifact &woody_axis_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		woody_axis_artifact,
		context("PLY-1.0", "dataset:tapered-primary-woody-axis"));
	assert(decoded.pointsSucceeded());
	assert(decoded.dataset().has_value());
	assert(decoded.dataset()->points().size() == 40u);
	const VegetationPointCloudReconstructionAdmissionService admission;
	const auto shoot_report = admission.evaluate(
		*decoded.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::ShootArchitecture, 32u,
			0.30, 0.0, false));
	assert(shoot_report.admitted());
	assert(shoot_report.woodyPointCount() == 40u);
	assert(shoot_report.foliagePointCount() == 0u);
	const VegetationPointCloudReconstructionJobFactory jobs;
	const auto woody_axis_job = jobs.create(
		"job:tapered-primary-woody-axis", *decoded.dataset(), shoot_report,
		"ProGen3D-PrimaryWoodyAxis-v1", 8u, 0u);
	assert(woody_axis_job.has_value());
	const VegetationPrimaryWoodyAxisReconstructionService reconstruction;
	const auto reconstructed = reconstruction.reconstruct(
		"woody-axis:tapered-primary", *decoded.dataset(), shoot_report,
		*woody_axis_job, woody_axis_policy());
	assert(reconstructed.succeeded());
	assert(reconstructed.qualityReport().has_value());
	assert(reconstructed.reconstruction().has_value());
	assert(reconstructed.reconstruction()->schemaVersion() ==
	       "ProGen3D-VegetationPrimaryWoodyAxisReconstruction-v1");
	assert(reconstructed.reconstruction()->scope() ==
	       VegetationWoodyAxisReconstructionScope::PrimaryAxisOnly);
	assert(!reconstructed.reconstruction()->representsCompleteBranchTopology());
	assert(reconstructed.reconstruction()->radiusStations().size() == 5u);
	assert(reconstructed.reconstruction()->cylinders().size() == 4u);
	const auto &axis =
		reconstructed.reconstruction()->principalAxisDirection();
	assert(std::abs(axis.x()) < 1.0e-12);
	assert(std::abs(axis.y()) < 1.0e-12);
	assert(std::abs(axis.z() - 1.0) < 1.0e-12);
	const std::vector<double> expected_radii = {0.20, 0.18, 0.16, 0.14, 0.12};
	for (std::size_t index = 0u; index < expected_radii.size(); ++index) {
		const auto &station =
			reconstructed.reconstruction()->radiusStations()[index];
		assert(station.stationIndex() == index);
		assert(station.supportingPointCount() == 6u);
		assert(std::abs(station.radiusMetres() - expected_radii[index]) <
		       1.0e-6);
	}
	const auto &quality = *reconstructed.qualityReport();
	assert(quality.trainingWoodyPointCount() == 30u);
	assert(quality.holdoutWoodyPointCount() == 10u);
	assert(quality.excludedTrainingOutlierCount() == 0u);
	assert(quality.radiusStationCount() == 5u);
	assert(quality.cylinderCount() == 4u);
	assert(std::abs(quality.woodyLabelFraction() - 1.0) < 1.0e-12);
	assert(quality.principalVarianceFraction() > 0.97);
	assert(quality.trainingSurfaceRmseMetres() < 1.0e-6);
	assert(quality.holdoutSurfaceRmseMetres() < 1.0e-6);
	assert(std::abs(quality.holdoutAxialCoverageFraction() - 1.0) <
	       1.0e-12);
	assert(std::abs(quality.taperViolationFraction()) < 1.0e-12);
	assert(reconstructed.reconstruction()->sourcePayloadSha256() ==
	       woody_axis_artifact.payloadSha256());
	assert(std::abs(
		       reconstructed.reconstruction()
			       ->woodyEvidenceBoundsMetres()
			       .extentZ() -
		       3.0) < 1.0e-12);

	const auto repeated = reconstruction.reconstruct(
		"woody-axis:tapered-primary", *decoded.dataset(), shoot_report,
		*woody_axis_job, woody_axis_policy());
	assert(repeated.succeeded());
	for (std::size_t index = 0u;
	     index < reconstructed.reconstruction()->radiusStations().size();
	     ++index) {
		const auto &first =
			reconstructed.reconstruction()->radiusStations()[index];
		const auto &second = repeated.reconstruction()->radiusStations()[index];
		assert(first.stationIdentifier() == second.stationIdentifier());
		assert(first.radiusMetres() == second.radiusMetres());
	}

	const auto strict_variance = reconstruction.reconstruct(
		"woody-axis:strict-variance", *decoded.dataset(), shoot_report,
		*woody_axis_job, woody_axis_policy(0.999));
	assert(!strict_variance.succeeded());
	assert(strict_variance.qualityReport().has_value());
	assert(has_woody_axis_issue(
		strict_variance, WoodyAxisIssueCode::QualityGateRejected));

	const auto cylinder_limited = reconstruction.reconstruct(
		"woody-axis:cylinder-limit", *decoded.dataset(), shoot_report,
		*woody_axis_job, woody_axis_policy(0.95, 3u));
	assert(!cylinder_limited.succeeded());
	assert(has_woody_axis_issue(
		cylinder_limited, WoodyAxisIssueCode::CylinderLimitExceeded));

	const auto insufficient_woody = reconstruction.reconstruct(
		"woody-axis:insufficient-woody", *decoded.dataset(), shoot_report,
		*woody_axis_job, woody_axis_policy(0.95, 8u, 41u));
	assert(!insufficient_woody.succeeded());
	assert(has_woody_axis_issue(
		insufficient_woody, WoodyAxisIssueCode::InsufficientWoodyEvidence));

	const auto unsupported_job = jobs.create(
		"job:tapered-primary-unsupported", *decoded.dataset(), shoot_report,
		"Unsupported-Primary-Axis-Algorithm", 8u, 0u);
	assert(unsupported_job.has_value());
	const auto unsupported_algorithm = reconstruction.reconstruct(
		"woody-axis:unsupported", *decoded.dataset(), shoot_report,
		*unsupported_job, woody_axis_policy());
	assert(!unsupported_algorithm.succeeded());
	assert(has_woody_axis_issue(
		unsupported_algorithm, WoodyAxisIssueCode::UnsupportedAlgorithm));

	const auto canopy_report = admission.evaluate(
		*decoded.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::CanopyOptics, 32u, 0.30,
			0.0, false));
	assert(canopy_report.admitted());
	const auto canopy_job = jobs.create(
		"job:tapered-primary-wrong-target", *decoded.dataset(), canopy_report,
		"ProGen3D-PrimaryWoodyAxis-v1", 8u, 0u);
	assert(canopy_job.has_value());
	const auto wrong_target = reconstruction.reconstruct(
		"woody-axis:wrong-target", *decoded.dataset(), canopy_report,
		*canopy_job, woody_axis_policy());
	assert(!wrong_target.succeeded());
	assert(has_woody_axis_issue(
		wrong_target, WoodyAxisIssueCode::WrongReconstructionTarget));

	const VegetationPointCloudReconstructionJob mismatched_hash_job(
		"ProGen3D-VegetationPointCloudReconstructionJob-v1",
		"job:tapered-primary-hash-mismatch",
		VegetationPointCloudReconstructionTarget::ShootArchitecture,
		decoded.dataset()->datasetIdentifier(), "not-the-source-hash",
		"ProGen3D-PrimaryWoodyAxis-v1", 8u, 0u);
	const auto mismatched_hash = reconstruction.reconstruct(
		"woody-axis:hash-mismatch", *decoded.dataset(), shoot_report,
		mismatched_hash_job, woody_axis_policy());
	assert(!mismatched_hash.succeeded());
	assert(has_woody_axis_issue(
		mismatched_hash, WoodyAxisIssueCode::SourceHashMismatch));
}

void verify_segmented_woody_branch_graph_reconstruction(
	const VegetationMeasuredSourceArtifact &segmented_tree_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		segmented_tree_artifact,
		context("PLY-1.0", "dataset:segmented-three-axis-tree"));
	assert(decoded.pointsSucceeded());
	assert(decoded.dataset().has_value());
	assert(decoded.dataset()->points().size() == 96u);
	const VegetationPointCloudReconstructionAdmissionService admission;
	const auto shoot_report = admission.evaluate(
		*decoded.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::ShootArchitecture, 80u,
			0.30, 0.0, false));
	assert(shoot_report.admitted());
	assert(shoot_report.woodyPointCount() == 96u);
	const VegetationPointCloudReconstructionJobFactory jobs;
	const auto graph_job = jobs.create(
		"job:segmented-three-axis-tree", *decoded.dataset(), shoot_report,
		"ProGen3D-SegmentedWoodyBranchGraph-v1", 16u, 0u);
	assert(graph_job.has_value());
	const auto segmentation = segmented_tree_assignments(*decoded.dataset());
	const VegetationSegmentedWoodyBranchGraphReconstructionService
		reconstruction_service;
	const auto reconstructed = reconstruction_service.reconstruct(
		"woody-graph:three-axis-tree", *decoded.dataset(), shoot_report,
		*graph_job, segmentation, woody_graph_policy());
	assert(reconstructed.succeeded());
	assert(reconstructed.segmentationReport().has_value());
	assert(reconstructed.qualityReport().has_value());
	assert(reconstructed.reconstruction().has_value());
	assert(reconstructed.segmentationReport()->assignedWoodyPointCount() ==
	       96u);
	assert(reconstructed.segmentationReport()->unassignedWoodyPointCount() ==
	       0u);
	assert(reconstructed.reconstruction()->schemaVersion() ==
	       "ProGen3D-VegetationWoodyBranchGraphReconstruction-v1");
	assert(reconstructed.reconstruction()->scope() ==
	       VegetationWoodyBranchGraphReconstructionScope::
		       CompleteForObservedWoodyEvidence);
	assert(!reconstructed.reconstruction()->representsCompleteBiologicalTree());
	assert(reconstructed.reconstruction()->axes().size() == 3u);
	assert(reconstructed.reconstruction()->connections().size() == 2u);
	assert(reconstructed.qualityReport()->axisCount() == 3u);
	assert(reconstructed.qualityReport()->cylinderCount() == 9u);
	assert(reconstructed.qualityReport()->connectionCount() == 2u);
	assert(reconstructed.qualityReport()->maximumBranchOrder() == 2u);
	assert(std::abs(
		       reconstructed.qualityReport()->assignmentCoverageFraction() -
		       1.0) < 1.0e-12);
	assert(reconstructed.qualityReport()->minimumPrincipalVarianceFraction() >
	       0.95);
	assert(
		reconstructed.qualityReport()->maximumTrainingSurfaceRmseMetres() <
		1.0e-6);
	assert(reconstructed.qualityReport()->maximumHoldoutSurfaceRmseMetres() <
	       1.0e-6);
	assert(std::abs(
		       reconstructed.qualityReport()
			       ->minimumHoldoutSurfaceCoverageFraction() -
		       1.0) < 1.0e-12);
	assert(reconstructed.qualityReport()->maximumAttachmentSurfaceGapMetres() <
	       0.081);
	assert(std::abs(
		       reconstructed.qualityReport()->maximumTaperViolationFraction()) <
	       1.0e-12);

	const auto &trunk = reconstructed.reconstruction()->axes()[0];
	const auto &primary_branch = reconstructed.reconstruction()->axes()[1];
	const auto &secondary_branch = reconstructed.reconstruction()->axes()[2];
	assert(trunk.axisIdentifier() == "axis:trunk");
	assert(primary_branch.axisIdentifier() == "axis:branch-primary");
	assert(secondary_branch.axisIdentifier() == "axis:branch-secondary");
	assert(trunk.branchOrder() == 0u);
	assert(primary_branch.branchOrder() == 1u);
	assert(secondary_branch.branchOrder() == 2u);
	assert(std::abs(trunk.axisDirection().z() - 1.0) < 1.0e-12);
	assert(std::abs(primary_branch.axisDirection().x() - 1.0) < 1.0e-12);
	assert(std::abs(secondary_branch.axisDirection().y() - 1.0) < 1.0e-12);
	assert(trunk.cylinders().size() == 4u);
	assert(primary_branch.cylinders().size() == 3u);
	assert(secondary_branch.cylinders().size() == 2u);
	assert(trunk.cylinders().front().parentCylinderIdentifier().empty());
	assert(!primary_branch.cylinders()
		        .front()
		        .parentCylinderIdentifier()
		        .empty());
	assert(!secondary_branch.cylinders()
		        .front()
		        .parentCylinderIdentifier()
		        .empty());
	assert(reconstructed.reconstruction()->sourcePayloadSha256() ==
	       segmented_tree_artifact.payloadSha256());

	const auto repeated = reconstruction_service.reconstruct(
		"woody-graph:three-axis-tree", *decoded.dataset(), shoot_report,
		*graph_job, segmentation, woody_graph_policy());
	assert(repeated.succeeded());
	for (std::size_t axis_index = 0u;
	     axis_index < reconstructed.reconstruction()->axes().size();
	     ++axis_index) {
		const auto &first_axis =
			reconstructed.reconstruction()->axes()[axis_index];
		const auto &second_axis = repeated.reconstruction()->axes()[axis_index];
		assert(first_axis.axisIdentifier() == second_axis.axisIdentifier());
		for (std::size_t cylinder_index = 0u;
		     cylinder_index < first_axis.cylinders().size(); ++cylinder_index) {
			assert(first_axis.cylinders()[cylinder_index].cylinderIdentifier() ==
			       second_axis.cylinders()[cylinder_index]
				       .cylinderIdentifier());
			assert(first_axis.cylinders()[cylinder_index].radiusMetres() ==
			       second_axis.cylinders()[cylinder_index].radiusMetres());
		}
	}

	const VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory
		artifact_factory;
	const auto qsm_artifact = artifact_factory.create(
		*reconstructed.reconstruction());
	const auto repeated_qsm_artifact = artifact_factory.create(
		*repeated.reconstruction());
	assert(qsm_artifact.has_value());
	assert(repeated_qsm_artifact.has_value());
	assert(qsm_artifact->payloadSha256() ==
	       repeated_qsm_artifact->payloadSha256());
	assert(qsm_artifact->sourcePayload() ==
	       repeated_qsm_artifact->sourcePayload());
	assert(qsm_artifact->sourcePayload().find(
		       segmented_tree_artifact.payloadSha256()) != std::string::npos);
	const VegetationCalibrationSubjectScope specimen_scope(
		VegetationCalibrationSubjectScopeKind::MeasuredSpecimen,
		PlantArchitecture::Tree, "specimen:segmented-three-axis-tree",
		std::string(), std::string(), "segmented-three-axis-tree");
	const auto adapted = VegetationQuantitativeStructureModelAdapter().adapt(
		*qsm_artifact, specimen_scope);
	assert(adapted.succeeded());
	assert(adapted.evidenceBundle().has_value());

	const auto incomplete_segmentation = segmented_tree_assignments(
		*decoded.dataset(), 0.99, false, false);
	const auto incomplete = reconstruction_service.reconstruct(
		"woody-graph:incomplete", *decoded.dataset(), shoot_report,
		*graph_job, incomplete_segmentation, woody_graph_policy());
	assert(!incomplete.succeeded());
	assert(has_woody_graph_issue(
		incomplete, WoodyGraphIssueCode::SegmentationIncomplete));

	const auto missing_assignment_segmentation = segmented_tree_assignments(
		*decoded.dataset(), 0.99, true, true);
	const auto missing_assignment = reconstruction_service.reconstruct(
		"woody-graph:missing-assignment", *decoded.dataset(), shoot_report,
		*graph_job, missing_assignment_segmentation, woody_graph_policy());
	assert(!missing_assignment.succeeded());
	assert(has_woody_graph_issue(
		missing_assignment, WoodyGraphIssueCode::SegmentationRejected));

	const auto low_confidence_segmentation = segmented_tree_assignments(
		*decoded.dataset(), 0.50, true, false);
	const auto low_confidence = reconstruction_service.reconstruct(
		"woody-graph:low-confidence", *decoded.dataset(), shoot_report,
		*graph_job, low_confidence_segmentation, woody_graph_policy());
	assert(!low_confidence.succeeded());
	assert(low_confidence.qualityReport().has_value());
	assert(has_woody_graph_issue(
		low_confidence, WoodyGraphIssueCode::QualityGateRejected));

	const auto strict_attachment = reconstruction_service.reconstruct(
		"woody-graph:strict-attachment", *decoded.dataset(), shoot_report,
		*graph_job, segmentation, woody_graph_policy(0.01));
	assert(!strict_attachment.succeeded());
	assert(strict_attachment.qualityReport().has_value());
	assert(has_woody_graph_issue(
		strict_attachment, WoodyGraphIssueCode::QualityGateRejected));

	const auto cylinder_limited = reconstruction_service.reconstruct(
		"woody-graph:cylinder-limit", *decoded.dataset(), shoot_report,
		*graph_job, segmentation, woody_graph_policy(0.10, 8u));
	assert(!cylinder_limited.succeeded());
	assert(has_woody_graph_issue(
		cylinder_limited, WoodyGraphIssueCode::CylinderLimitExceeded));

	const auto unsupported_job = jobs.create(
		"job:segmented-three-axis-unsupported", *decoded.dataset(),
		shoot_report, "Unsupported-Segmented-Graph-Algorithm", 16u, 0u);
	assert(unsupported_job.has_value());
	const auto unsupported = reconstruction_service.reconstruct(
		"woody-graph:unsupported", *decoded.dataset(), shoot_report,
		*unsupported_job, segmentation, woody_graph_policy());
	assert(!unsupported.succeeded());
	assert(has_woody_graph_issue(
		unsupported, WoodyGraphIssueCode::UnsupportedAlgorithm));
}

void verify_automated_woody_segmentation_ensemble(
	const VegetationMeasuredSourceArtifact &segmented_tree_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto decoded = ingestion.ingest(
		segmented_tree_artifact,
		context("PLY-1.0", "dataset:automated-three-axis-tree"));
	assert(decoded.pointsSucceeded());
	assert(decoded.dataset().has_value());
	const VegetationPointCloudReconstructionAdmissionService admission;
	const auto shoot_report = admission.evaluate(
		*decoded.dataset(),
		VegetationPointCloudReconstructionAdmissionPolicy(
			VegetationPointCloudReconstructionTarget::ShootArchitecture, 80u,
			0.30, 0.0, false));
	assert(shoot_report.admitted());
	const VegetationPointCloudReconstructionJobFactory jobs;
	const auto ensemble_job = jobs.create(
		"job:automated-three-axis-tree", *decoded.dataset(), shoot_report,
		"ProGen3D-AutomatedWoodySegmentationEnsemble-v1", 20u,
		20260828u);
	assert(ensemble_job.has_value());
	const VegetationAutomatedWoodySegmentationEnsembleService service;
	const auto reconstructed = service.reconstruct(
		"woody-ensemble:three-axis-tree", *decoded.dataset(), shoot_report,
		*ensemble_job, automated_segmentation_parameter_sets(),
		automated_graph_policy(), automated_sensitivity_policy());
	if (!reconstructed.succeeded()) {
		for (const auto &issue : reconstructed.issues()) {
			std::cerr << "Automated ensemble issue: " << issue.message()
			          << std::endl;
		}
		for (const auto &evaluation : reconstructed.evaluations()) {
			std::cerr << "Candidate "
			          << evaluation.parameterSet().parameterIdentifier()
			          << " segmentation="
			          << evaluation.segmentationReport().succeeded()
			          << " graph=" << evaluation.acceptedForSensitivity()
			          << std::endl;
			for (const auto &issue : evaluation.segmentationReport().issues()) {
				std::cerr << "  segmentation: " << issue.message() << std::endl;
			}
			if (evaluation.segmentationReport().candidate().has_value()) {
				const auto &candidate =
					*evaluation.segmentationReport().candidate();
				for (const auto &axis : candidate.axisCandidates()) {
					const auto assignment_count = std::count_if(
						candidate.segmentation().pointAssignments().begin(),
						candidate.segmentation().pointAssignments().end(),
						[&](const VegetationWoodyPointSegmentAssignment &assignment) {
							return assignment.axisIdentifier() ==
							       axis.axisIdentifier();
						});
					std::cerr << "  axis " << axis.axisIdentifier()
					          << " order=" << axis.branchOrder()
					          << " assignments=" << assignment_count
					          << " station="
					          << axis.axialStationSpacingMetres() << std::endl;
				}
			}
			if (evaluation.graphReport().has_value()) {
				for (const auto &issue : evaluation.graphReport()->issues()) {
					std::cerr << "  graph axis=" << issue.axisIdentifier()
					          << ": " << issue.message() << std::endl;
				}
				if (evaluation.graphReport()->qualityReport().has_value()) {
					const auto &quality =
						*evaluation.graphReport()->qualityReport();
					std::cerr << "  quality axes=" << quality.axisCount()
					          << " cylinders=" << quality.cylinderCount()
					          << " order=" << quality.maximumBranchOrder()
					          << " confidence="
					          << quality.minimumAssignmentConfidence()
					          << " variance="
					          << quality.minimumPrincipalVarianceFraction()
					          << " train_rmse="
					          << quality.maximumTrainingSurfaceRmseMetres()
					          << " holdout_rmse="
					          << quality.maximumHoldoutSurfaceRmseMetres()
					          << " coverage="
					          << quality.minimumHoldoutSurfaceCoverageFraction()
					          << " attachment="
					          << quality.maximumAttachmentSurfaceGapMetres()
					          << " taper="
					          << quality.maximumTaperViolationFraction()
					          << std::endl;
				}
			}
		}
		if (reconstructed.sensitivityReport().has_value()) {
			for (const auto &reason :
			     reconstructed.sensitivityReport()->rejectionReasons()) {
				std::cerr << "Sensitivity: " << reason << std::endl;
			}
		}
	}
	assert(reconstructed.succeeded());
	assert(reconstructed.evaluations().size() == 3u);
	assert(reconstructed.sensitivityReport().has_value());
	assert(reconstructed.selectedCandidate().has_value());
	assert(reconstructed.selectedReconstruction().has_value());
	assert(reconstructed.sensitivityReport()->acceptedCandidateCount() >= 2u);
	assert(reconstructed.sensitivityReport()
		       ->minimumSelectedAssignmentAgreementFraction() >= 0.88);
	assert(std::abs(
		       reconstructed.sensitivityReport()->axisCountConsensusFraction() -
		       1.0) < 1.0e-12);
	assert(std::abs(
		       reconstructed.sensitivityReport()->branchOrderConsensusFraction() -
		       1.0) < 1.0e-12);
	assert(reconstructed.selectedCandidate()->coverSets().size() >= 7u);
	assert(reconstructed.selectedCandidate()->coverConnections().size() + 1u ==
	       reconstructed.selectedCandidate()->coverSets().size());
	assert(reconstructed.selectedCandidate()->axisCandidates().size() == 3u);
	assert(reconstructed.selectedCandidate()
		       ->segmentation()
		       .segmentationAlgorithmIdentifier() ==
	       "ProGen3D-WoodyCoverSetSegmentation-v1");
	assert(reconstructed.selectedCandidate()
		       ->segmentation()
		       .completeForObservedWoodyPoints());
	assert(reconstructed.selectedCandidate()
		       ->segmentation()
		       .pointAssignments()
		       .size() == 96u);
	assert(reconstructed.selectedReconstruction()->axes().size() == 3u);
	assert(reconstructed.selectedReconstruction()->connections().size() == 2u);
	assert(reconstructed.selectedReconstruction()
		       ->qualityReport()
		       .maximumBranchOrder() == 2u);
	assert(!reconstructed.selectedReconstruction()
		        ->representsCompleteBiologicalTree());
	assert(reconstructed.selectedReconstruction()->sourcePayloadSha256() ==
	       segmented_tree_artifact.payloadSha256());

	const auto repeated = service.reconstruct(
		"woody-ensemble:three-axis-tree", *decoded.dataset(), shoot_report,
		*ensemble_job, automated_segmentation_parameter_sets(),
		automated_graph_policy(), automated_sensitivity_policy());
	assert(repeated.succeeded());
	assert(repeated.sensitivityReport()->selectedParameterIdentifier() ==
	       reconstructed.sensitivityReport()->selectedParameterIdentifier());
	assert(repeated.selectedCandidate()
		       ->segmentation()
		       .pointAssignments()
		       .size() ==
	       reconstructed.selectedCandidate()
		       ->segmentation()
		       .pointAssignments()
		       .size());
	for (std::size_t index = 0u;
	     index < repeated.selectedCandidate()
		             ->segmentation()
		             .pointAssignments()
		             .size();
	     ++index) {
		const auto &first = reconstructed.selectedCandidate()
			                    ->segmentation()
			                    .pointAssignments()[index];
		const auto &second = repeated.selectedCandidate()
			                     ->segmentation()
			                     .pointAssignments()[index];
		assert(first.sourcePointIndex() == second.sourcePointIndex());
		assert(first.axisIdentifier() == second.axisIdentifier());
		assert(first.assignmentConfidence() == second.assignmentConfidence());
	}
	const VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory
		artifact_factory;
	const auto qsm_artifact = artifact_factory.create(
		*reconstructed.selectedReconstruction());
	const auto repeated_qsm_artifact = artifact_factory.create(
		*repeated.selectedReconstruction());
	assert(qsm_artifact.has_value());
	assert(repeated_qsm_artifact.has_value());
	assert(qsm_artifact->payloadSha256() ==
	       repeated_qsm_artifact->payloadSha256());
	const VegetationCalibrationSubjectScope specimen_scope(
		VegetationCalibrationSubjectScopeKind::MeasuredSpecimen,
		PlantArchitecture::Tree, "specimen:automated-three-axis-tree",
		std::string(), std::string(), "automated-three-axis-tree");
	const auto adapted = VegetationQuantitativeStructureModelAdapter().adapt(
		*qsm_artifact, specimen_scope);
	assert(adapted.succeeded());

	const VegetationWoodyCoverSetSegmentationService candidate_service;
	const VegetationWoodySegmentationParameterSet disconnected_parameters(
		"disconnected", 0.65, 0.25, 1u, 3u, 64u, 8u, 0.55, 0.60,
		2u, 0.20);
	const auto disconnected = candidate_service.segment(
		"candidate:disconnected", *decoded.dataset(), shoot_report,
		*ensemble_job, disconnected_parameters);
	assert(!disconnected.succeeded());
	assert(has_segmentation_candidate_issue(
		disconnected, SegmentationCandidateIssueCode::DisconnectedCoverGraph));

	const VegetationWoodySegmentationParameterSet ambiguous_parameters(
		"ambiguous", 0.65, 1.85, 1u, 3u, 64u, 8u, 0.55, 0.99, 2u,
		0.20);
	const auto ambiguous = candidate_service.segment(
		"candidate:ambiguous", *decoded.dataset(), shoot_report,
		*ensemble_job, ambiguous_parameters);
	assert(!ambiguous.succeeded());
	assert(ambiguous.candidate().has_value());
	assert(has_segmentation_candidate_issue(
		ambiguous, SegmentationCandidateIssueCode::AmbiguousPointAssignment));

	const auto unstable = service.reconstruct(
		"woody-ensemble:unstable", *decoded.dataset(), shoot_report,
		*ensemble_job, automated_segmentation_parameter_sets(),
		automated_graph_policy(), automated_sensitivity_policy(0.99));
	assert(!unstable.succeeded());
	assert(unstable.sensitivityReport().has_value());
	assert(has_automated_segmentation_issue(
		unstable,
		AutomatedSegmentationIssueCode::SensitivityGateRejected));

	const auto unsupported_job = jobs.create(
		"job:automated-three-axis-unsupported", *decoded.dataset(),
		shoot_report, "Unsupported-Automated-Segmentation", 20u, 0u);
	assert(unsupported_job.has_value());
	const auto unsupported = service.reconstruct(
		"woody-ensemble:unsupported", *decoded.dataset(), shoot_report,
		*unsupported_job, automated_segmentation_parameter_sets(),
		automated_graph_policy(), automated_sensitivity_policy());
	assert(!unsupported.succeeded());
	assert(has_automated_segmentation_issue(
		unsupported, AutomatedSegmentationIssueCode::UnsupportedAlgorithm));
}

void verify_rejections(
	const VegetationMeasuredSourceArtifact &ply_artifact,
	const VegetationMeasuredSourceArtifact &las_artifact,
	const VegetationMeasuredSourceArtifact &binary_ply_artifact)
{
	const VegetationPointCloudIngestionService ingestion;

	std::string duplicate_ply = ply_artifact.sourcePayload();
	const std::string last_record =
		"1.0 0.5 3.0 14000 44000 16000 290 3 1 2 0.92";
	const std::string first_record =
		"0.0 0.0 0.0 18000 12000 7000 420 1 1 1 0.99";
	const std::size_t last_position = duplicate_ply.find(last_record);
	assert(last_position != std::string::npos);
	duplicate_ply.replace(last_position, last_record.size(), first_record);
	const auto duplicate = ingestion.ingest(
		source_artifact("source:ply:duplicate", "application/ply", duplicate_ply),
		context("PLY-1.0", "dataset:ply-duplicate"));
	assert(!duplicate.pointsSucceeded());
	assert(has_issue(duplicate, IssueCode::DuplicatePoint));

	const auto limited = ingestion.ingest(
		ply_artifact, context("PLY-1.0", "dataset:ply-limited", 5u));
	assert(!limited.pointsSucceeded());
	assert(has_issue(limited, IssueCode::PointCountLimitExceeded));

	std::string truncated_binary_ply = binary_ply_artifact.sourcePayload();
	truncated_binary_ply.pop_back();
	const auto truncated_binary = ingestion.ingest(
		source_artifact(
			"source:ply:binary-truncated", "application/ply",
			truncated_binary_ply),
		context("PLY-1.0", "dataset:ply-binary-truncated"));
	assert(!truncated_binary.pointsSucceeded());
	assert(has_issue(truncated_binary, IssueCode::TruncatedPointData));

	std::string stale_payload = ply_artifact.sourcePayload();
	stale_payload.push_back('\n');
	const VegetationMeasuredSourceArtifact stale_hash(
		ply_artifact.schemaVersion(), ply_artifact.sourceIdentifier(),
		ply_artifact.sourceCitation(), ply_artifact.sourceLocator(),
		ply_artifact.mediaType(), ply_artifact.coordinateSystem(),
		ply_artifact.unitSystem(), ply_artifact.acquisitionMethod(),
		ply_artifact.uncertaintyStatement(), stale_payload,
		ply_artifact.payloadSha256());
	const auto stale = ingestion.ingest(
		stale_hash, context("PLY-1.0", "dataset:ply-stale"));
	assert(!stale.pointsSucceeded());
	assert(has_issue(stale, IssueCode::SourceHashMismatch));

	std::string truncated_las = las_artifact.sourcePayload();
	truncated_las.pop_back();
	const auto truncated = ingestion.ingest(
		source_artifact(
			"source:las:truncated", "application/vnd.las", truncated_las),
		context("LAS-1.4", "dataset:las-truncated"));
	assert(!truncated.pointsSucceeded());
	assert(has_issue(truncated, IssueCode::TruncatedPointData));

	std::string las_15_payload = las_artifact.sourcePayload();
	las_15_payload[25u] = static_cast<char>(5u);
	const auto las_15 = ingestion.ingest(
		source_artifact("source:las:1.5", "application/vnd.las", las_15_payload),
		context("LAS-1.5", "dataset:las-1.5"));
	assert(!las_15.pointsSucceeded());
	assert(has_issue(las_15, IssueCode::UnsupportedSourceSchemaVersion));

	const auto laz = ingestion.ingest(
		source_artifact("source:laz:missing", "application/vnd.laz", "LAZ"),
		context("LAZ-1.4", "dataset:laz"));
	assert(has_issue(laz, IssueCode::MissingRequiredDependency));
	const auto e57 = ingestion.ingest(
		source_artifact("source:e57:missing", "model/e57", "ASTM E57"),
		context("E57-ASTM-E2807", "dataset:e57"));
	assert(has_issue(e57, IssueCode::MissingRequiredDependency));
	const auto unknown = ingestion.ingest(
		source_artifact("source:unknown", "application/octet-stream", "unknown"),
		context("Unknown", "dataset:unknown"));
	assert(has_issue(unknown, IssueCode::UnsupportedMediaType));
}

void verify_determinism(
	const VegetationMeasuredSourceArtifact &ply_artifact)
{
	const VegetationPointCloudIngestionService ingestion;
	const auto first = ingestion.ingest(
		ply_artifact, context("PLY-1.0", "dataset:ply-deterministic"));
	const auto second = ingestion.ingest(
		ply_artifact, context("PLY-1.0", "dataset:ply-deterministic"));
	assert(first.pointsSucceeded());
	assert(second.pointsSucceeded());
	assert(first.dataset()->points().size() == second.dataset()->points().size());
	for (std::size_t index = 0u; index < first.dataset()->points().size();
	     ++index) {
		const auto &first_position =
			first.dataset()->points()[index].positionMetres();
		const auto &second_position =
			second.dataset()->points()[index].positionMetres();
		assert(first_position.x() == second_position.x());
		assert(first_position.y() == second_position.y());
		assert(first_position.z() == second_position.z());
	}
	const VegetationPointCloudReconstructionAdmissionPolicy policy(
		VegetationPointCloudReconstructionTarget::CanopyOptics, 10u, 0.5, 0.0,
		true);
	const VegetationPointCloudReconstructionAdmissionService admission;
	const auto first_report = admission.evaluate(*first.dataset(), policy);
	const auto second_report = admission.evaluate(*second.dataset(), policy);
	assert(first_report.admitted() == second_report.admitted());
	assert(first_report.pointCount() == second_report.pointCount());
	assert(first_report.duplicatePointCount() ==
	       second_report.duplicatePointCount());
}

}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string fixture_directory = argv[1];
	const auto ply_artifact = source_artifact(
		"source:ply:tree-canopy", "application/ply",
		read_text_file(fixture_directory + "/tree_canopy_ascii_ply_v1.ply"));
	const auto las_artifact = source_artifact(
		"source:las:tree-canopy", "application/vnd.las",
		read_hex_file(
			fixture_directory + "/tree_canopy_las_1_4_format_6.hex"));
	const auto binary_little_ply_artifact = source_artifact(
		"source:ply:tree-canopy-binary-little", "application/ply",
		read_hex_file(
			fixture_directory +
			"/tree_canopy_binary_little_endian_ply_v1.hex"));
	const auto binary_big_ply_artifact = source_artifact(
		"source:ply:tree-canopy-binary-big", "application/ply",
		read_hex_file(
			fixture_directory +
			"/tree_canopy_binary_big_endian_ply_v1.hex"));
	const auto woody_axis_artifact = source_artifact(
		"source:ply:tapered-primary-woody-axis", "application/ply",
		read_text_file(
			fixture_directory +
			"/tapered_primary_woody_axis_ascii_ply_v1.ply"));
	const auto segmented_tree_artifact = source_artifact(
		"source:ply:segmented-three-axis-tree", "application/ply",
		read_text_file(
			fixture_directory +
			"/segmented_three_axis_tree_ascii_ply_v1.ply"));

	verify_capability_catalog();
	const auto ply = verify_ply_decode(ply_artifact);
	const auto las = verify_las_decode(las_artifact);
	const auto binary_little = verify_binary_ply_decode(
		binary_little_ply_artifact,
		VegetationPointCloudEncoding::BinaryLittleEndian,
		"dataset:ply-binary-little");
	const auto binary_big = verify_binary_ply_decode(
		binary_big_ply_artifact, VegetationPointCloudEncoding::BinaryBigEndian,
		"dataset:ply-binary-big");
	assert(binary_little.dataset()->points().size() ==
	       binary_big.dataset()->points().size());
	for (std::size_t index = 0u;
	     index < binary_little.dataset()->points().size(); ++index) {
		const auto &little_position =
			binary_little.dataset()->points()[index].positionMetres();
		const auto &big_position =
			binary_big.dataset()->points()[index].positionMetres();
		assert(little_position.x() == big_position.x());
		assert(little_position.y() == big_position.y());
		assert(little_position.z() == big_position.z());
	}
	verify_reconstruction_admission(ply, las);
	verify_primary_woody_axis_reconstruction(woody_axis_artifact);
	verify_segmented_woody_branch_graph_reconstruction(
		segmented_tree_artifact);
	verify_automated_woody_segmentation_ensemble(segmented_tree_artifact);
	verify_rejections(ply_artifact, las_artifact, binary_little_ply_artifact);
	verify_determinism(ply_artifact);

	std::cout << "Vegetation point-cloud D3A through D3B2C2 checks passed."
	          << std::endl;
	return 0;
}
