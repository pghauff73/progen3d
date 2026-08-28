#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationCalibrationEvidenceBundleParsingResult.h"
#include "vegetation/service/VegetationCalibrationEvidenceBindingService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleParsingService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.h"
#include "vegetation/service/VegetationCalibrationEvidenceBundleValidationService.h"
#include "vegetation/service/VegetationCalibrationPayloadHashService.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

using MeasurementKind = VegetationCalibrationMeasurementKind;
using ValidationCode = VegetationCalibrationEvidenceBundleValidationCode;

BuildingObjectSpatialProfile spatial_profile()
{
	return BuildingObjectSpatialProfile(
		SpatialObjectId("SMB_VEGETATION_EVIDENCE_TREE"),
		BuildingSpatialManifestationKind::PhysicalAggregate,
		BuildingPlacementPolicyKind::ConstraintSolved,
		BuildingCollisionBehaviorKind::SoftBoundary,
		BuildingObjectExtentSet({
			BuildingExtentDescriptor(
				BuildingExtentKind::Visual,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"grammar-sha:evidence-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Collision,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"collision:evidence-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Growth,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::VegetationGrowthEnvelope,
				"growth:evidence-tree"),
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
		"BVO.EvidenceTree",
		"Evidence Tree",
		PlantArchitecture::Tree,
		spatial_profile(),
		VegetationRepresentationFidelityProfile(
			95.0, 95.0, 95.0, 95.0, 95.0, 95.0),
		VegetationTriangleDistributionPolicy(24000u));
}

VegetationCalibrationMeasurement decimal_measurement(
	MeasurementKind kind,
	const std::string &unit,
	double value)
{
	return VegetationCalibrationMeasurement(
		kind,
		vegetationCalibrationDomainName(
			vegetationCalibrationMeasurementDomain(kind)),
		VegetationCalibrationMeasurementValue(value),
		unit,
		"specimen:tree-001",
		"Fixture uncertainty is bounded and explicit.",
		{"source:measurements"});
}

VegetationCalibrationMeasurement count_measurement(
	MeasurementKind kind,
	std::size_t value)
{
	return VegetationCalibrationMeasurement(
		kind,
		vegetationCalibrationDomainName(
			vegetationCalibrationMeasurementDomain(kind)),
		VegetationCalibrationMeasurementValue(value),
		"count",
		"specimen:tree-001",
		"Fixture uncertainty is bounded and explicit.",
		{"source:measurements"});
}

VegetationCalibrationMeasurement text_measurement(
	MeasurementKind kind,
	const std::string &unit,
	const std::string &value)
{
	return VegetationCalibrationMeasurement(
		kind,
		vegetationCalibrationDomainName(
			vegetationCalibrationMeasurementDomain(kind)),
		VegetationCalibrationMeasurementValue(value),
		unit,
		"specimen:tree-001",
		"Fixture uncertainty is bounded and explicit.",
		{"source:measurements"});
}

VegetationCalibrationMeasurement list_measurement(
	MeasurementKind kind,
	std::vector<std::string> values)
{
	return VegetationCalibrationMeasurement(
		kind,
		vegetationCalibrationDomainName(
			vegetationCalibrationMeasurementDomain(kind)),
		VegetationCalibrationMeasurementValue(std::move(values)),
		"identifier-list",
		"specimen:tree-001",
		"Fixture uncertainty is bounded and explicit.",
		{"source:measurements"});
}

std::vector<VegetationCalibrationMeasurement> complete_measurements()
{
	return {
		text_measurement(
			MeasurementKind::ScientificName, "text", "Lophostemon confertus"),
		text_measurement(
			MeasurementKind::SpecimenIdentifier, "identifier", "tree-001"),
		text_measurement(
			MeasurementKind::ShootTopologyArtifactIdentifier,
			"identifier", "qsm:tree-001"),
		count_measurement(MeasurementKind::MaximumBranchOrder, 5u),
		text_measurement(
			MeasurementKind::RootGraphIdentifier,
			"identifier", "root-graph:tree-001"),
		decimal_measurement(MeasurementKind::MaximumRootDepthMetres, "m", 1.8),
		decimal_measurement(
			MeasurementKind::MaximumRootRadialSpreadMetres, "m", 3.5),
		count_measurement(MeasurementKind::MaximumRootOrder, 4u),
		decimal_measurement(MeasurementKind::LeafAreaIndex, "dimensionless", 3.2),
		decimal_measurement(MeasurementKind::CrownGapFraction, "fraction", 0.27),
		decimal_measurement(
			MeasurementKind::MeanLeafInclinationDegrees, "degree", 42.0),
		text_measurement(
			MeasurementKind::SeasonalScheduleArtifactIdentifier,
			"identifier", "phenology:tree-001"),
		decimal_measurement(
			MeasurementKind::MassDensityKilogramsPerCubicMetre, "kg/m3", 680.0),
		decimal_measurement(
			MeasurementKind::ElasticModulusPascals, "Pa", 9.0e9),
		decimal_measurement(MeasurementKind::DampingRatio, "dimensionless", 0.04),
		decimal_measurement(
			MeasurementKind::DragCoefficient, "dimensionless", 0.9),
		text_measurement(
			MeasurementKind::EnvironmentalResponseArtifactIdentifier,
			"identifier", "response:tree-001"),
		text_measurement(
			MeasurementKind::OntologyIdentifier,
			"identifier", "PlantOntology"),
		text_measurement(
			MeasurementKind::OntologyReleaseIdentifier,
			"identifier", "2026-08-28"),
		list_measurement(
			MeasurementKind::AnatomicalEntityIdentifiers,
			{"PO:0000003", "PO:0025029", "PO:0009008"}),
		list_measurement(
			MeasurementKind::DevelopmentStageIdentifiers,
			{"PO:0007132", "PO:0007134"}),
		decimal_measurement(
			MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram,
			"m2/kg", 14.0),
		decimal_measurement(
			MeasurementKind::LeafDryMatterContentKilogramsPerKilogram,
			"fraction", 0.38),
		decimal_measurement(
			MeasurementKind::LeafNitrogenContentKilogramsPerKilogram,
			"fraction", 0.021),
		decimal_measurement(
			MeasurementKind::MaximumMatureHeightMetres, "m", 18.0),
		decimal_measurement(
			MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal,
			"mmol/m2/s/MPa", 5.4),
		decimal_measurement(
			MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal,
			"kg/MPa", 2.1),
		decimal_measurement(
			MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals,
			"MPa", -3.2),
		decimal_measurement(
			MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals,
			"MPa", -2.4),
		decimal_measurement(
			MeasurementKind::LeafToSapwoodAreaRatio,
			"dimensionless", 4200.0),
		decimal_measurement(MeasurementKind::ReferenceHeightMetres, "m", 12.0),
		decimal_measurement(
			MeasurementKind::ReferenceHorizontalRadiusMetres, "m", 4.8),
		decimal_measurement(
			MeasurementKind::ReferenceSupportingAxisDiameterMetres, "m", 0.42),
		text_measurement(
			MeasurementKind::SizeRelationshipModelIdentifier,
			"identifier", "allometry:open-grown-v1"),
		decimal_measurement(
			MeasurementKind::MinimumRootableVolumeCubicMetres, "m3", 34.0),
		decimal_measurement(
			MeasurementKind::MinimumRootableDepthMetres, "m", 1.2),
		decimal_measurement(
			MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre,
			"kg/m3", 1500.0),
		decimal_measurement(
			MeasurementKind::MinimumAirFilledPorosityFraction,
			"fraction", 0.12),
		decimal_measurement(
			MeasurementKind::MinimumAvailableWaterCapacityFraction,
			"fraction", 0.18),
		text_measurement(
			MeasurementKind::GrammarEvidenceIdentifier,
			"identifier", "grammar-sha:evidence-tree"),
		text_measurement(
			MeasurementKind::CameraEvidenceIdentifier,
			"identifier", "camera-evidence:evidence-tree"),
	};
}

std::vector<VegetationCalibrationSourceReference> source_references()
{
	return {
		VegetationCalibrationSourceReference(
			"source:measurements",
			"Measured fixture dataset",
			"fixture://measurements/tree-001",
			std::string(64u, 'a')),
		VegetationCalibrationSourceReference(
			"source:method",
			"Fixture acquisition method",
			"fixture://methods/calibration-v1",
			std::string(64u, 'b')),
	};
}

VegetationCalibrationSubjectScope measured_scope(
	const std::string &specimen_identifier = "tree-001",
	PlantArchitecture architecture = PlantArchitecture::Tree)
{
	return VegetationCalibrationSubjectScope(
		VegetationCalibrationSubjectScopeKind::MeasuredSpecimen,
		architecture,
		"specimen:" + specimen_identifier,
		"Lophostemon confertus",
		std::string(),
		specimen_identifier);
}

VegetationCalibrationEvidenceBundle unsigned_bundle(
	std::vector<VegetationCalibrationMeasurement> measurements,
	VegetationCalibrationSubjectScope scope = measured_scope(),
	std::vector<VegetationCalibrationSourceReference> sources = source_references())
{
	return VegetationCalibrationEvidenceBundle(
		"ProGen3D-VegetationCalibrationEvidenceBundle-v1",
		"bundle:tree-001:v1",
		std::move(scope),
		"ProGen3DWorldXYZMetres",
		"SI",
		"TLS, field measurement, and traceable laboratory observations",
		"Fine roots, occluded shoots, and temporal plasticity remain uncertain.",
		std::move(sources),
		std::move(measurements),
		std::string());
}

VegetationCalibrationEvidenceBundle signed_bundle(
	std::vector<VegetationCalibrationMeasurement> measurements =
		complete_measurements(),
	VegetationCalibrationSubjectScope scope = measured_scope(),
	std::vector<VegetationCalibrationSourceReference> sources = source_references())
{
	return VegetationCalibrationPayloadHashService().attachCanonicalPayloadSha256(
		unsigned_bundle(
			std::move(measurements), std::move(scope), std::move(sources)));
}

bool has_validation_issue(
	const VegetationCalibrationEvidenceBundleValidationReport &report,
	ValidationCode expected_code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[expected_code](
			const VegetationCalibrationEvidenceBundleValidationIssue &issue) {
			return issue.code() == expected_code;
		});
}

void verify_exact_round_trip_and_order_independence()
{
	const VegetationCalibrationEvidenceBundle bundle = signed_bundle();
	const VegetationCalibrationEvidenceBundleSerializationService serialization;
	const std::string json = serialization.serialize(bundle);
	const VegetationCalibrationEvidenceBundleParsingResult parsed =
		VegetationCalibrationEvidenceBundleParsingService().parse(json);
	assert(parsed.succeeded());
	assert(serialization.serialize(*parsed.bundle()) == json);
	assert(parsed.bundle()->payloadSha256() == bundle.payloadSha256());

	std::vector<VegetationCalibrationMeasurement> reversed_measurements =
		complete_measurements();
	std::reverse(reversed_measurements.begin(), reversed_measurements.end());
	std::vector<VegetationCalibrationSourceReference> reversed_sources =
		source_references();
	std::reverse(reversed_sources.begin(), reversed_sources.end());
	const VegetationCalibrationEvidenceBundle reversed = signed_bundle(
		std::move(reversed_measurements), measured_scope(),
		std::move(reversed_sources));
	assert(reversed.payloadSha256() == bundle.payloadSha256());
	assert(serialization.serialize(reversed) == json);

	const VegetationCalibrationEvidenceBundleParsingResult malformed =
		VegetationCalibrationEvidenceBundleParsingService().parse("{not-json");
	assert(!malformed.succeeded());
}

void verify_complete_binding_and_readiness_transition()
{
	const BuildingVegetationObjectModel model = archetype_model();
	const VegetationCalibrationEvidenceBundle bundle = signed_bundle();
	const VegetationCalibrationEvidenceBundleValidationReport validation =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, bundle);
	assert(validation.passed());

	const VegetationCalibrationEvidenceBindingReport report =
		VegetationCalibrationEvidenceBindingService().bind(model, bundle);
	assert(report.succeeded());
	assert(report.readyDomainsBeforeBinding().size() == 1u);
	assert(report.readyDomainsBeforeBinding().front() ==
	       VegetationCalibrationDomain::BuildingPlacement);
	assert(report.readyDomainsAfterBinding().size() == 14u);
	assert(report.observations().size() == complete_measurements().size());
	for (const VegetationCalibrationEvidenceBindingObservation &observation :
	     report.observations()) {
		assert(observation.disposition() ==
		       VegetationCalibrationEvidenceBindingDisposition::Accepted);
	}
	assert(report.boundProfile()->identity().specimenIdentifier() == "tree-001");
	assert(report.boundProfile()->architecturalTopology().maximumBranchOrder() ==
	       5u);
	assert(report.boundProfile()->rootArchitecture().rootGraphIdentifier() ==
	       "root-graph:tree-001");
	assert(report.boundProfile()->hydraulics().supportsCalibratedWaterTransport());
	const std::vector<std::string> &source_identifiers =
		report.boundProfile()->evidence().sourceIdentifiers();
	assert(std::find(
		       source_identifiers.begin(), source_identifiers.end(),
		       "bundle:tree-001:v1") != source_identifiers.end());
	assert(std::find(
		       source_identifiers.begin(), source_identifiers.end(),
		       "source:measurements") != source_identifiers.end());
	assert(std::find(
		       source_identifiers.begin(), source_identifiers.end(),
		       "Lophostemon confertus") == source_identifiers.end());
	assert(std::find(
		       source_identifiers.begin(), source_identifiers.end(),
		       "root-graph:tree-001") == source_identifiers.end());
	assert(!model.biologicalProfile().identity().isTaxonomicallyCalibrated());
}

void verify_partial_bundle_is_deferred()
{
	const BuildingVegetationObjectModel model = archetype_model();
	const VegetationCalibrationEvidenceBundle partial = signed_bundle({
		decimal_measurement(MeasurementKind::MaximumRootDepthMetres, "m", 1.8),
	});
	const VegetationCalibrationEvidenceBindingReport report =
		VegetationCalibrationEvidenceBindingService().bind(model, partial);
	assert(report.succeeded());
	assert(report.observations().size() == 1u);
	assert(report.observations().front().disposition() ==
	       VegetationCalibrationEvidenceBindingDisposition::Deferred);
	assert(report.readyDomainsAfterBinding() ==
	       report.readyDomainsBeforeBinding());
	assert(!report.boundProfile()->rootArchitecture()
	            .hasCalibratedRootArchitecture());
}

void verify_validation_rejections()
{
	const BuildingVegetationObjectModel model = archetype_model();

	std::vector<VegetationCalibrationMeasurement> duplicates =
		complete_measurements();
	duplicates.push_back(duplicates.front());
	const auto duplicate_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, signed_bundle(std::move(duplicates)));
	assert(has_validation_issue(
		duplicate_report, ValidationCode::DuplicateMeasurement));

	const VegetationCalibrationEvidenceBundle stale =
		signed_bundle().withPayloadSha256(std::string(64u, '0'));
	const auto stale_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, stale);
	assert(has_validation_issue(stale_report, ValidationCode::PayloadHashMismatch));

	std::vector<VegetationCalibrationMeasurement> unknown_domain =
		complete_measurements();
	unknown_domain.front() = VegetationCalibrationMeasurement(
		unknown_domain.front().kind(),
		"UnknownDomain",
		unknown_domain.front().value(),
		unknown_domain.front().unit(),
		unknown_domain.front().observationScope(),
		unknown_domain.front().uncertaintyStatement(),
		unknown_domain.front().evidenceSourceIdentifiers());
	const auto unknown_domain_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, signed_bundle(std::move(unknown_domain)));
	assert(has_validation_issue(
		unknown_domain_report, ValidationCode::UnknownMeasurementDomain));

	std::vector<VegetationCalibrationMeasurement> unsupported_unit =
		complete_measurements();
	unsupported_unit[5] = decimal_measurement(
		MeasurementKind::MaximumRootDepthMetres, "feet", 6.0);
	const auto unsupported_unit_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, signed_bundle(std::move(unsupported_unit)));
	assert(has_validation_issue(
		unsupported_unit_report, ValidationCode::UnsupportedMeasurementUnit));

	const auto architecture_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model,
			signed_bundle(
				complete_measurements(),
				measured_scope("tree-001", PlantArchitecture::Shrub)));
	assert(has_validation_issue(
		architecture_report, ValidationCode::ArchitectureMismatch));

	std::vector<VegetationCalibrationSourceReference> missing_hash_sources =
		source_references();
	missing_hash_sources.front() = VegetationCalibrationSourceReference(
		"source:measurements",
		"Measured fixture dataset",
		"fixture://measurements/tree-001",
		std::string());
	const auto missing_hash_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model,
			signed_bundle(
				complete_measurements(), measured_scope(),
				std::move(missing_hash_sources)));
	assert(has_validation_issue(
		missing_hash_report, ValidationCode::MissingEvidenceHash));

	std::vector<VegetationCalibrationMeasurement> contradictory =
		complete_measurements();
	contradictory[1] = text_measurement(
		MeasurementKind::SpecimenIdentifier, "identifier", "tree-002");
	const auto contradictory_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, signed_bundle(std::move(contradictory)));
	assert(has_validation_issue(
		contradictory_report,
		ValidationCode::ContradictoryIdentityMeasurement));

	const VegetationCalibrationEvidenceBundle non_finite = unsigned_bundle({
		decimal_measurement(
			MeasurementKind::MaximumRootDepthMetres,
			"m",
			std::numeric_limits<double>::infinity()),
	});
	const auto non_finite_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			model, non_finite);
	assert(has_validation_issue(
		non_finite_report, ValidationCode::NonFiniteMeasurementValue));
}

void verify_mismatched_specimen_is_rejected()
{
	const BuildingVegetationObjectModel archetype = archetype_model();
	const VegetationCalibrationEvidenceBindingReport initial_binding =
		VegetationCalibrationEvidenceBindingService().bind(
			archetype, signed_bundle());
	assert(initial_binding.succeeded());
	const BuildingVegetationObjectModel measured_model(
		archetype.modelIdentifier(),
		archetype.displayName(),
		archetype.plantArchitecture(),
		archetype.spatialProfile(),
		archetype.fidelityProfile(),
		archetype.triangleDistributionPolicy(),
		*initial_binding.boundProfile());
	const VegetationCalibrationEvidenceBundle mismatched = signed_bundle(
		{
			decimal_measurement(
				MeasurementKind::MaximumRootDepthMetres, "m", 1.9),
		},
		measured_scope("tree-002"));
	const auto validation =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			measured_model, mismatched);
	assert(has_validation_issue(
		validation, ValidationCode::ContradictoryIdentity));
	const VegetationCalibrationEvidenceBindingReport binding =
		VegetationCalibrationEvidenceBindingService().bind(
			measured_model, mismatched);
	assert(!binding.succeeded());
	assert(!binding.boundProfile().has_value());
	assert(binding.observations().front().disposition() ==
	       VegetationCalibrationEvidenceBindingDisposition::Rejected);
}

}

int main()
{
	verify_exact_round_trip_and_order_independence();
	verify_complete_binding_and_readiness_transition();
	verify_partial_bundle_is_deferred();
	verify_validation_rejections();
	verify_mismatched_specimen_is_rejected();
	std::cout << "Vegetation calibration evidence bundle checks passed.\n";
	return 0;
}
