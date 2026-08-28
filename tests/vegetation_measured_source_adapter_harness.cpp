#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/service/VegetationBiomechanicalMaterialTestAdapter.h"
#include "vegetation/service/VegetationCalibrationEvidenceBindingService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleParsingService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleValidationService.h"
#include "vegetation/service/VegetationCalibrationReadinessEvaluationService.h"
#include "vegetation/service/VegetationCanopyObservationAdapter.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"
#include "vegetation/service/VegetationMeasuredCoordinateNormalizationService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"
#include "vegetation/service/VegetationPhenologyObservationSeriesAdapter.h"
#include "vegetation/service/VegetationQuantitativeStructureModelAdapter.h"
#include "vegetation/service/VegetationRootArchitectureGraphAdapter.h"

#include <json/json.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using IssueCode = VegetationMeasuredSourceAdapterIssueCode;
using Disposition = VegetationMeasuredSourceRecordDisposition;
using MeasurementKind = VegetationCalibrationMeasurementKind;

std::string read_text_file(const std::string &path)
{
	std::ifstream stream(path);
	assert(stream.good());
	return std::string(
		std::istreambuf_iterator<char>(stream),
		std::istreambuf_iterator<char>());
}

std::string compact_json(const Json::Value &value)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	writer["commentStyle"] = "None";
	return Json::writeString(writer, value);
}

Json::Value parse_json(const std::string &text)
{
	Json::CharReaderBuilder builder;
	Json::Value value;
	std::string errors;
	std::istringstream stream(text);
	assert(Json::parseFromStream(builder, stream, &value, &errors));
	return value;
}

VegetationCalibrationSubjectScope measured_tree_scope()
{
	return VegetationCalibrationSubjectScope(
		VegetationCalibrationSubjectScopeKind::MeasuredSpecimen,
		PlantArchitecture::Tree,
		"specimen:tree-001",
		"Lophostemon confertus",
		std::string(),
		"tree-001");
}

BuildingObjectSpatialProfile spatial_profile()
{
	return BuildingObjectSpatialProfile(
		SpatialObjectId("SMB_MEASURED_SOURCE_TREE"),
		BuildingSpatialManifestationKind::PhysicalAggregate,
		BuildingPlacementPolicyKind::ConstraintSolved,
		BuildingCollisionBehaviorKind::SoftBoundary,
		BuildingObjectExtentSet({
			BuildingExtentDescriptor(
				BuildingExtentKind::Visual,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"grammar-sha:measured-source-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Collision,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"collision:measured-source-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Growth,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::VegetationGrowthEnvelope,
				"growth:measured-source-tree"),
		}),
		BuildingOrientationProfile(
			BuildingOrientationPolicyKind::KeepUpright,
			BuildingRotationalSymmetryKind::ContinuousYaw,
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)),
		{},
		{"landscape_support_graph", "vegetation_growth_graph"},
		1u,
		7u);
}

BuildingVegetationObjectModel archetype_model()
{
	return BuildingVegetationObjectModel(
		"BVO.MeasuredSourceTree",
		"Measured Source Tree",
		PlantArchitecture::Tree,
		spatial_profile(),
		VegetationRepresentationFidelityProfile(
			95.0, 95.0, 95.0, 95.0, 95.0, 95.0),
		VegetationTriangleDistributionPolicy(24000u));
}

BuildingVegetationObjectModel with_profile(
	const BuildingVegetationObjectModel &model,
	VegetationBiologicalProfile profile)
{
	return BuildingVegetationObjectModel(
		model.modelIdentifier(),
		model.displayName(),
		model.plantArchitecture(),
		model.spatialProfile(),
		model.fidelityProfile(),
		model.triangleDistributionPolicy(),
		std::move(profile));
}

VegetationMeasuredSourceArtifact source_artifact(
	const std::string &source_identifier,
	const std::string &media_type,
	std::string payload)
{
	const VegetationMeasuredSourceArtifact unsigned_artifact(
		"ProGen3D-VegetationMeasuredSourceArtifact-v1",
		source_identifier,
		"Frozen deterministic measured-source fixture",
		"fixture://vegetation-measured-sources/" + source_identifier,
		media_type,
		"LocalPlantXYZ-ZUp",
		"DeclaredPerField",
		"Frozen measured-source adapter fixture",
		"Fixture measurements retain explicit source and adapter uncertainty.",
		std::move(payload),
		std::string());
	return VegetationMeasuredSourceArtifactHashService().attachPayloadSha256(
		unsigned_artifact);
}

const VegetationCalibrationMeasurement &measurement(
	const VegetationCalibrationEvidenceBundle &bundle,
	MeasurementKind kind)
{
	const auto found = std::find_if(
		bundle.measurements().begin(), bundle.measurements().end(),
		[kind](const VegetationCalibrationMeasurement &candidate) {
			return candidate.kind() == kind;
		});
	assert(found != bundle.measurements().end());
	return *found;
}

bool has_issue(
	const VegetationMeasuredSourceAdapterReport &report,
	IssueCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](const VegetationMeasuredSourceAdapterIssue &issue) {
			return issue.code() == expected_code;
		});
}

bool has_disposition(
	const VegetationMeasuredSourceAdapterReport &report,
	Disposition expected_disposition)
{
	return std::any_of(
		report.observations().begin(), report.observations().end(),
		[expected_disposition](
			const VegetationMeasuredSourceRecordObservation &observation) {
			return observation.disposition() == expected_disposition;
		});
}

void verify_bundle_contract(
	const BuildingVegetationObjectModel &model,
	const VegetationCalibrationEvidenceBundle &bundle)
{
	const auto validation =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, bundle);
	assert(validation.passed());
	const VegetationCalibrationEvidenceBundleSerializationService serialization;
	const std::string json = serialization.serialize(bundle);
	const auto parsed =
		VegetationCalibrationEvidenceBundleParsingService().parse(json);
	assert(parsed.succeeded());
	assert(serialization.serialize(*parsed.bundle()) == json);
	assert(bundle.coordinateSystem() == "LocalPlantXYZ-ZUp");
	for (const VegetationCalibrationMeasurement &current : bundle.measurements()) {
		assert(current.uncertaintyStatement() ==
		       "Fixture measurements retain explicit source and adapter uncertainty.");
		assert(current.evidenceSourceIdentifiers().size() == 1u);
		assert(current.evidenceSourceIdentifiers().front() ==
		       bundle.sourceReferences().front().sourceIdentifier());
	}
}

BuildingVegetationObjectModel bind_bundle(
	const BuildingVegetationObjectModel &model,
	const VegetationCalibrationEvidenceBundle &bundle)
{
	const VegetationCalibrationEvidenceBindingReport report =
		VegetationCalibrationEvidenceBindingService().bind(model, bundle);
	assert(report.succeeded());
	return with_profile(model, *report.boundProfile());
}

void verify_unit_normalization()
{
	const VegetationMeasurementUnitNormalizationService normalization;
	const auto centimetres = normalization.normalize(
		240.0, "cm", VegetationMeasurementQuantityKind::LengthMetres);
	assert(centimetres.succeeded());
	assert(std::abs(*centimetres.normalizedValue() - 2.4) < 1.0e-12);
	const auto density = normalization.normalize(
		0.72, "g/cm3",
		VegetationMeasurementQuantityKind::MassDensityKilogramsPerCubicMetre);
	assert(density.succeeded());
	assert(std::abs(*density.normalizedValue() - 720.0) < 1.0e-12);
	const auto angle = normalization.normalize(
		0.7853981633974483, "rad",
		VegetationMeasurementQuantityKind::AngleDegrees);
	assert(angle.succeeded());
	assert(angle.canonicalUnit() == "degree");
	assert(std::abs(*angle.normalizedValue() - 45.0) < 1.0e-10);
	assert(!normalization
		        .normalize(
			        1.0, "feet",
			        VegetationMeasurementQuantityKind::LengthMetres)
		        .succeeded());
}

void verify_coordinate_normalization()
{
	const VegetationMeasuredCoordinateNormalizationService normalization;
	const auto y_up = normalization.normalize(
		VegetationMeasuredPoint3d(2.0, 5.0, -3.0),
		"LocalPlantXZY-YUp");
	assert(y_up.succeeded());
	assert(y_up.canonicalCoordinateSystem() == "LocalPlantXYZ-ZUp");
	assert(y_up.normalizedPoint()->x() == 2.0);
	assert(y_up.normalizedPoint()->y() == -3.0);
	assert(y_up.normalizedPoint()->z() == 5.0);
	assert(!normalization
		        .normalize(
			        VegetationMeasuredPoint3d(0.0, 0.0, 0.0),
			        "ProGen3DWorldXYZ-ZUp")
		        .succeeded());
}

struct AdapterFixtures
{
	VegetationMeasuredSourceArtifact qsm;
	VegetationMeasuredSourceArtifact root;
	VegetationMeasuredSourceArtifact canopy;
	VegetationMeasuredSourceArtifact phenology;
	VegetationMeasuredSourceArtifact biomechanics;
};

AdapterFixtures load_fixtures(const std::string &fixture_directory)
{
	return AdapterFixtures{
		source_artifact(
			"source:qsm:tree-001",
			"application/vnd.progen3d.qsm+json",
			read_text_file(fixture_directory + "/tree_qsm_v1.json")),
		source_artifact(
			"source:root-graph:tree-001",
			"application/vnd.progen3d.root-architecture+json",
			read_text_file(fixture_directory + "/tree_root_graph_v1.json")),
		source_artifact(
			"source:canopy:tree-001",
			"application/vnd.progen3d.canopy-observation+json",
			read_text_file(fixture_directory + "/tree_canopy_observation_v1.json")),
		source_artifact(
			"source:phenology:tree-001",
			"application/vnd.progen3d.phenology-series+json",
			read_text_file(fixture_directory + "/tree_phenology_series_v1.json")),
		source_artifact(
			"source:biomechanics:tree-001",
			"application/vnd.progen3d.biomechanical-test+json",
			read_text_file(fixture_directory + "/tree_biomechanical_test_v1.json")),
	};
}

void verify_successful_adapters_and_binding(const AdapterFixtures &fixtures)
{
	const VegetationCalibrationSubjectScope scope = measured_tree_scope();
	const BuildingVegetationObjectModel initial = archetype_model();

	const VegetationMeasuredSourceAdapterReport qsm =
		VegetationQuantitativeStructureModelAdapter().adapt(fixtures.qsm, scope);
	assert(qsm.succeeded());
	assert(has_disposition(qsm, Disposition::Accepted));
	assert(measurement(*qsm.evidenceBundle(), MeasurementKind::MaximumBranchOrder)
		       .value()
		       .countValue() == 1u);
	verify_bundle_contract(initial, *qsm.evidenceBundle());
	const std::string qsm_json =
		VegetationCalibrationEvidenceBundleSerializationService().serialize(
			*qsm.evidenceBundle());
	const auto qsm_repeat =
		VegetationQuantitativeStructureModelAdapter().adapt(fixtures.qsm, scope);
	assert(qsm_repeat.succeeded());
	assert(VegetationCalibrationEvidenceBundleSerializationService().serialize(
		       *qsm_repeat.evidenceBundle()) == qsm_json);

	const VegetationMeasuredSourceAdapterReport root =
		VegetationRootArchitectureGraphAdapter().adapt(fixtures.root, scope);
	assert(root.succeeded());
	const double root_depth =
		*measurement(
			 *root.evidenceBundle(), MeasurementKind::MaximumRootDepthMetres)
			 .value()
			 .decimalValue();
	assert(std::abs(root_depth - 0.94) < 1.0e-12);
	const double root_spread =
		*measurement(
			 *root.evidenceBundle(),
			 MeasurementKind::MaximumRootRadialSpreadMetres)
			 .value()
			 .decimalValue();
	assert(std::abs(root_spread - std::sqrt(2.15 * 2.15 + 0.62 * 0.62)) <
	       1.0e-12);
	verify_bundle_contract(initial, *root.evidenceBundle());

	const VegetationMeasuredSourceAdapterReport canopy =
		VegetationCanopyObservationAdapter().adapt(fixtures.canopy, scope);
	assert(canopy.succeeded());
	assert(has_disposition(canopy, Disposition::Accepted));
	assert(has_disposition(canopy, Disposition::Deferred));
	assert(std::abs(
		       *measurement(
			        *canopy.evidenceBundle(), MeasurementKind::CrownGapFraction)
			        .value()
			        .decimalValue() -
		       0.24) < 1.0e-12);
	assert(std::abs(
		       *measurement(
			        *canopy.evidenceBundle(),
			        MeasurementKind::MeanLeafInclinationDegrees)
			        .value()
			        .decimalValue() -
		       45.0) < 1.0e-10);
	verify_bundle_contract(initial, *canopy.evidenceBundle());

	const VegetationMeasuredSourceAdapterReport phenology =
		VegetationPhenologyObservationSeriesAdapter().adapt(
			fixtures.phenology, scope);
	assert(phenology.succeeded());
	assert(phenology.observations().size() == 3u);
	verify_bundle_contract(initial, *phenology.evidenceBundle());

	const VegetationMeasuredSourceAdapterReport biomechanics =
		VegetationBiomechanicalMaterialTestAdapter().adapt(
			fixtures.biomechanics, scope);
	assert(biomechanics.succeeded());
	assert(std::abs(
		       *measurement(
			        *biomechanics.evidenceBundle(),
			        MeasurementKind::MassDensityKilogramsPerCubicMetre)
			        .value()
			        .decimalValue() -
		       720.0) < 1.0e-12);
	assert(std::abs(
		       *measurement(
			        *biomechanics.evidenceBundle(),
			        MeasurementKind::ElasticModulusPascals)
			        .value()
			        .decimalValue() -
		       8.5e9) < 1.0e-3);
	verify_bundle_contract(initial, *biomechanics.evidenceBundle());

	BuildingVegetationObjectModel calibrated =
		bind_bundle(initial, *qsm.evidenceBundle());
	calibrated = bind_bundle(calibrated, *root.evidenceBundle());
	calibrated = bind_bundle(calibrated, *canopy.evidenceBundle());
	calibrated = bind_bundle(calibrated, *phenology.evidenceBundle());
	calibrated = bind_bundle(calibrated, *biomechanics.evidenceBundle());
	const VegetationCalibrationReadinessReport readiness =
		VegetationCalibrationReadinessEvaluationService().evaluate(
			calibrated,
			{
				VegetationCalibrationDomain::ShootTopology,
				VegetationCalibrationDomain::RootArchitecture,
				VegetationCalibrationDomain::CanopyOptics,
				VegetationCalibrationDomain::Phenology,
				VegetationCalibrationDomain::Biomechanics,
			});
	assert(readiness.allRequestedDomainsReady());
}

void verify_rejections(const AdapterFixtures &fixtures)
{
	const VegetationCalibrationSubjectScope scope = measured_tree_scope();
	const VegetationMeasuredSourceArtifact unsupported_coordinates(
		fixtures.qsm.schemaVersion(),
		fixtures.qsm.sourceIdentifier(),
		fixtures.qsm.sourceCitation(),
		fixtures.qsm.sourceLocator(),
		fixtures.qsm.mediaType(),
		"ProGen3DWorldXYZ-ZUp",
		fixtures.qsm.unitSystem(),
		fixtures.qsm.acquisitionMethod(),
		fixtures.qsm.uncertaintyStatement(),
		fixtures.qsm.sourcePayload(),
		fixtures.qsm.payloadSha256());
	const auto coordinate_report =
		VegetationQuantitativeStructureModelAdapter().adapt(
			unsupported_coordinates, scope);
	assert(!coordinate_report.succeeded());
	assert(has_issue(
		coordinate_report, IssueCode::UnsupportedCoordinateSystem));

	const VegetationMeasuredSourceArtifact stale =
		fixtures.qsm.withPayloadSha256(std::string(64u, '0'));
	const auto stale_report =
		VegetationQuantitativeStructureModelAdapter().adapt(stale, scope);
	assert(!stale_report.succeeded());
	assert(has_issue(stale_report, IssueCode::SourceHashMismatch));

	Json::Value invalid_qsm = parse_json(fixtures.qsm.sourcePayload());
	invalid_qsm["cylinders"][2u]["parent_identifier"] = "missing-parent";
	const auto invalid_qsm_report =
		VegetationQuantitativeStructureModelAdapter().adapt(
			source_artifact(
				"source:qsm:invalid-parent",
				"application/vnd.progen3d.qsm+json",
				compact_json(invalid_qsm)),
			scope);
	assert(!invalid_qsm_report.succeeded());
	assert(has_issue(invalid_qsm_report, IssueCode::InvalidParentReference));
	assert(has_disposition(invalid_qsm_report, Disposition::Rejected));

	Json::Value incomplete_root = parse_json(fixtures.root.sourcePayload());
	incomplete_root["segments"].resize(1u);
	const auto incomplete_root_report =
		VegetationRootArchitectureGraphAdapter().adapt(
			source_artifact(
				"source:root-graph:incomplete",
				"application/vnd.progen3d.root-architecture+json",
				compact_json(incomplete_root)),
			scope);
	assert(!incomplete_root_report.succeeded());
	assert(has_issue(incomplete_root_report, IssueCode::IncompleteGraph));

	Json::Value mismatched_canopy = parse_json(fixtures.canopy.sourcePayload());
	mismatched_canopy["plant_architecture"] = "Shrub";
	const auto mismatch_report = VegetationCanopyObservationAdapter().adapt(
		source_artifact(
			"source:canopy:mismatch",
			"application/vnd.progen3d.canopy-observation+json",
			compact_json(mismatched_canopy)),
		scope);
	assert(!mismatch_report.succeeded());
	assert(has_issue(mismatch_report, IssueCode::ArchitectureMismatch));

	Json::Value invalid_phenology = parse_json(fixtures.phenology.sourcePayload());
	std::swap(invalid_phenology["observations"][0u],
	          invalid_phenology["observations"][1u]);
	const auto phenology_report =
		VegetationPhenologyObservationSeriesAdapter().adapt(
			source_artifact(
				"source:phenology:unordered",
				"application/vnd.progen3d.phenology-series+json",
				compact_json(invalid_phenology)),
			scope);
	assert(!phenology_report.succeeded());
	assert(has_issue(phenology_report, IssueCode::InvalidObservationOrder));

	Json::Value invalid_biomechanics =
		parse_json(fixtures.biomechanics.sourcePayload());
	invalid_biomechanics["elastic_modulus"]["unit"] = "psi";
	const auto biomechanics_report =
		VegetationBiomechanicalMaterialTestAdapter().adapt(
			source_artifact(
				"source:biomechanics:unsupported-unit",
				"application/vnd.progen3d.biomechanical-test+json",
				compact_json(invalid_biomechanics)),
			scope);
	assert(!biomechanics_report.succeeded());
	assert(has_issue(biomechanics_report, IssueCode::UnsupportedUnit));
}

}

int main(int argc, char **argv)
{
	assert(argc == 2);
	verify_unit_normalization();
	verify_coordinate_normalization();
	const AdapterFixtures fixtures = load_fixtures(argv[1]);
	verify_successful_adapters_and_binding(fixtures);
	verify_rejections(fixtures);
	std::cout << "Vegetation measured source adapter checks passed.\n";
	return 0;
}
