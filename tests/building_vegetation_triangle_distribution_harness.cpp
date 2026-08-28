#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/service/VegetationBiologicalProfileValidationService.h"
#include "vegetation/service/VegetationCalibrationReadinessEvaluationService.h"
#include "vegetation/service/VegetationRepresentationFidelityEvaluationService.h"
#include "vegetation/service/VegetationTriangleDistributionService.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

VegetationTriangleImportance importance(
	double silhouette,
	double projected_area,
	double curvature,
	double topology,
	double motion,
	double material_boundary,
	double visibility,
	double biological_area)
{
	return VegetationTriangleImportance(
		silhouette, projected_area, curvature, topology,
		motion, material_boundary, visibility, biological_area);
}

std::vector<VegetationTriangleRegion> representative_tree_regions()
{
	return {
		VegetationTriangleRegion(
			"root_flare", VegetationTriangleRegionRole::RootFlare,
			1200u, 300u, 3.0,
			importance(0.72, 0.45, 0.82, 0.90, 0.08, 0.52, 0.78, 0.35)),
		VegetationTriangleRegion(
			"primary_stem", VegetationTriangleRegionRole::PrimaryStem,
			2200u, 500u, 8.0,
			importance(0.86, 0.68, 0.72, 1.00, 0.25, 0.58, 0.96, 0.42)),
		VegetationTriangleRegion(
			"branch_junctions", VegetationTriangleRegionRole::BranchJunction,
			1800u, 500u, 5.0,
			importance(0.74, 0.58, 1.00, 1.00, 0.34, 0.46, 0.82, 0.38)),
		VegetationTriangleRegion(
			"crown_silhouette", VegetationTriangleRegionRole::FoliageSilhouette,
			6000u, 1100u, 42.0,
			importance(1.00, 0.96, 0.48, 0.52, 0.82, 0.34, 1.00, 0.90)),
		VegetationTriangleRegion(
			"crown_interior", VegetationTriangleRegionRole::FoliageInterior,
			6000u, 500u, 78.0,
			importance(0.18, 0.58, 0.24, 0.34, 0.70, 0.18, 0.42, 1.00)),
		VegetationTriangleRegion(
			"flowers", VegetationTriangleRegionRole::FlowerOrFruit,
			1000u, 200u, 4.0,
			importance(0.52, 0.40, 0.56, 0.24, 0.62, 1.00, 0.74, 0.48))};
}

const VegetationTriangleAllocation &allocation_for(
	const VegetationTriangleDistributionSnapshot &snapshot,
	const std::string &region_identifier)
{
	const auto found = std::find_if(
		snapshot.allocations().begin(), snapshot.allocations().end(),
		[&](const VegetationTriangleAllocation &allocation) {
			return allocation.regionIdentifier() == region_identifier;
		});
	assert(found != snapshot.allocations().end());
	return *found;
}

void verify_exact_deterministic_distribution()
{
	const VegetationTriangleDistributionPolicy policy(6000u, 124u, true);
	const VegetationTriangleDistributionService service;
	const std::vector<VegetationTriangleRegion> regions =
		representative_tree_regions();
	const VegetationTriangleDistributionResult first = service.distribute(
		VegetationTriangleDistributionRequest(policy, regions));
	assert(first.succeeded());
	const VegetationTriangleDistributionSnapshot &snapshot = *first.snapshot();
	assert(snapshot.sourceTriangleCount() == 18200u);
	assert(snapshot.allocatedTriangleCount() == 6000u);
	assert(snapshot.allocations().size() == regions.size());
	assert(snapshot.evidenceHash() != 0u);

	std::map<std::string, std::size_t> semantic_minimums;
	for (const VegetationTriangleRegion &region : regions) {
		semantic_minimums.emplace(
			region.regionIdentifier(), region.minimumTriangleCount());
	}
	std::size_t allocated_sum = 0u;
	for (const VegetationTriangleAllocation &allocation : snapshot.allocations()) {
		allocated_sum += allocation.allocatedTriangleCount();
		assert(allocation.allocatedTriangleCount() >=
		       semantic_minimums.at(allocation.regionIdentifier()));
		assert(allocation.allocatedTriangleCount() <=
		       allocation.sourceTriangleCount());
		assert(allocation.meshletCount() ==
		       (allocation.allocatedTriangleCount() + 123u) / 124u);
	}
	assert(allocated_sum == 6000u);
	assert(allocation_for(snapshot, "crown_silhouette").allocatedTriangleCount() >
	       allocation_for(snapshot, "crown_interior").allocatedTriangleCount());
	assert(allocation_for(snapshot, "crown_silhouette")
	           .organLinearAreaPreservationScale() > 1.0);
	assert(allocation_for(snapshot, "primary_stem")
	           .organLinearAreaPreservationScale() == 1.0);

	std::vector<VegetationTriangleRegion> reversed = regions;
	std::reverse(reversed.begin(), reversed.end());
	const VegetationTriangleDistributionResult second = service.distribute(
		VegetationTriangleDistributionRequest(policy, reversed));
	assert(second.succeeded());
	assert(second.snapshot()->evidenceHash() == snapshot.evidenceHash());
	for (const VegetationTriangleAllocation &allocation : snapshot.allocations()) {
		assert(allocation_for(*second.snapshot(), allocation.regionIdentifier())
		           .allocatedTriangleCount() == allocation.allocatedTriangleCount());
	}
}

void verify_full_resolution_distribution()
{
	const std::vector<VegetationTriangleRegion> regions =
		representative_tree_regions();
	const VegetationTriangleDistributionResult result =
		VegetationTriangleDistributionService().distribute(
			VegetationTriangleDistributionRequest(
				VegetationTriangleDistributionPolicy(18200u), regions));
	assert(result.succeeded());
	for (const VegetationTriangleAllocation &allocation :
	     result.snapshot()->allocations()) {
		assert(allocation.allocatedTriangleCount() ==
		       allocation.sourceTriangleCount());
		assert(std::fabs(allocation.organLinearAreaPreservationScale() - 1.0) <
		       1.0e-12);
	}
}

void verify_fail_closed_requests()
{
	const VegetationTriangleDistributionService service;
	const std::vector<VegetationTriangleRegion> regions =
		representative_tree_regions();
	assert(!service.distribute(VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy(1000u), regions)).succeeded());
	assert(!service.distribute(VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy(20000u), regions)).succeeded());
	assert(!service.distribute(VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy(6000u, 0u), regions)).succeeded());

	std::vector<VegetationTriangleRegion> duplicates = regions;
	duplicates.push_back(regions.front());
	assert(!service.distribute(VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy(6000u), duplicates)).succeeded());

	std::vector<VegetationTriangleRegion> invalid_importance = regions;
	invalid_importance.emplace_back(
		"invalid", VegetationTriangleRegionRole::FoliageInterior,
		100u, 10u, 1.0,
		importance(1.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	assert(!service.distribute(VegetationTriangleDistributionRequest(
		VegetationTriangleDistributionPolicy(6010u), invalid_importance)).succeeded());
}

bool has_biological_validation_issue(
	const VegetationBiologicalProfileValidationReport &report,
	VegetationBiologicalProfileValidationCode code)
{
	return std::any_of(
		report.issues().begin(), report.issues().end(),
		[code](const VegetationBiologicalProfileValidationIssue &issue) {
			return issue.code() == code;
		});
}

BuildingObjectSpatialProfile calibrated_spatial_profile()
{
	return BuildingObjectSpatialProfile(
		SpatialObjectId("SMB_VEGETATION_CALIBRATED_TREE"),
		BuildingSpatialManifestationKind::PhysicalAggregate,
		BuildingPlacementPolicyKind::ConstraintSolved,
		BuildingCollisionBehaviorKind::SoftBoundary,
		BuildingObjectExtentSet({
			BuildingExtentDescriptor(
				BuildingExtentKind::Visual,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"grammar-sha:calibrated-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Collision,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::AuthoredBoundary,
				"collision-evidence:calibrated-tree"),
			BuildingExtentDescriptor(
				BuildingExtentKind::Growth,
				BuildingExtentApplicability::Applicable,
				BoundaryRepresentationKind::AxisAlignedBounding,
				BuildingExtentSourceKind::VegetationGrowthEnvelope,
				"growth-envelope:calibrated-tree"),
		}),
		BuildingOrientationProfile(
			BuildingOrientationPolicyKind::KeepUpright,
			BuildingRotationalSymmetryKind::ContinuousYaw,
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)),
		{},
		{
			"landscape_support_graph",
			"vegetation_growth_graph",
			"root_zone_constraint_graph",
		},
		1u,
		42u);
}

VegetationBiologicalProfile calibrated_biological_profile()
{
	return VegetationBiologicalProfile(
		PlantIdentityProfile(
			PlantIdentityResolutionKind::MeasuredSpecimen,
			PlantArchitecture::Tree,
			"ArchitecturalCanopyTree",
			"Lophostemon confertus",
			std::string(),
			{"specimen:tree-001", "identity:herbarium-record"},
			"tree-001"),
		PlantArchitecturalTopologyProfile(
			VegetationObjectCompositionKind::IndividualPlant,
			{
				PlantArchitecturalScale::WholePlant,
				PlantArchitecturalScale::Axis,
				PlantArchitecturalScale::GrowthUnit,
				PlantArchitecturalScale::Metamer,
				PlantArchitecturalScale::Organ,
				PlantArchitecturalScale::GeometryRegion,
				PlantArchitecturalScale::RepresentationCluster,
			},
			{
				PlantTopologyRelationshipKind::Decomposition,
				PlantTopologyRelationshipKind::Succession,
				PlantTopologyRelationshipKind::Branching,
				PlantTopologyRelationshipKind::Attachment,
			},
			true,
			true,
			{"qsm:tree-001"},
			5u),
		PlantPhenologyProfile(
			PlantDevelopmentState::Mature,
			{
				PlantDevelopmentState::Juvenile,
				PlantDevelopmentState::Mature,
				PlantDevelopmentState::Flowering,
				PlantDevelopmentState::Fruiting,
				PlantDevelopmentState::Senescent,
				PlantDevelopmentState::Dormant,
			},
			{"phenology:tree-001-brisbane"}),
		PlantRootArchitectureProfile(
			PlantRootRepresentationKind::ExplicitRootGraph,
			1.8,
			3.5,
			4u,
			{"root-observation:tree-001"},
			"root-graph:tree-001"),
		PlantCanopyOpticalProfile(
			3.2,
			0.27,
			42.0,
			{"tls-canopy:tree-001"}),
		PlantBiomechanicalProfile(
			680.0,
			9.0e9,
			0.04,
			0.9,
			{"mechanical-test:tree-001"}),
		PlantEnvironmentalResponseProfile(
			{
				PlantEnvironmentalDriver::Light,
				PlantEnvironmentalDriver::Water,
				PlantEnvironmentalDriver::Temperature,
				PlantEnvironmentalDriver::Wind,
				PlantEnvironmentalDriver::Soil,
				PlantEnvironmentalDriver::AvailableSpace,
			},
			{"response-model:tree-001"}),
		VegetationEvidenceProfile(
			"DeterministicGrammarAndMeasuredExtents",
			"MeasuredSpecimen",
			"TLS occlusion and unobserved fine roots remain explicit uncertainties.",
			{"grammar-sha:calibrated-tree", "camera-evidence:calibrated-tree"}),
		PlantSemanticAnnotationProfile(
			"PlantOntology",
			"2026-08-28",
			{"PO:0000003", "PO:0025029", "PO:0009008"},
			{"PO:0007134", "PO:0007132"},
			{"ontology-crosswalk:tree-001"}),
		PlantFunctionalTraitProfile(
			14.0,
			0.38,
			0.021,
			18.0,
			{"trait-observation:tree-001"}),
		PlantHydraulicProfile(
			5.4,
			2.1,
			-3.2,
			-2.4,
			4200.0,
			{"hydraulic-observation:tree-001"}),
		PlantSizeAllometryProfile(
			12.0,
			4.8,
			0.42,
			"allometry:open-grown-lophostemon-v1",
			{"allometry-observation:tree-001"}),
		VegetationSubstrateRequirementProfile(
			34.0,
			1.2,
			1500.0,
			0.12,
			0.18,
			{"substrate-requirement:tree-001"}));
}

void verify_biological_profile_validation_and_readiness()
{
	const BuildingVegetationObjectModel calibrated_model(
		"BVO.CalibratedTree",
		"Calibrated Tree",
		PlantArchitecture::Tree,
		calibrated_spatial_profile(),
		VegetationRepresentationFidelityProfile(
			95.0, 95.0, 95.0, 95.0, 95.0, 95.0),
		VegetationTriangleDistributionPolicy(24000u),
		calibrated_biological_profile());
	const VegetationBiologicalProfileValidationReport validation =
		VegetationBiologicalProfileValidationService().validate(calibrated_model);
	assert(validation.passed());

	const std::vector<VegetationCalibrationDomain> domains = {
		VegetationCalibrationDomain::TaxonomicIdentity,
		VegetationCalibrationDomain::ShootTopology,
		VegetationCalibrationDomain::RootArchitecture,
		VegetationCalibrationDomain::CanopyOptics,
		VegetationCalibrationDomain::Phenology,
		VegetationCalibrationDomain::Biomechanics,
		VegetationCalibrationDomain::EnvironmentalResponse,
		VegetationCalibrationDomain::SemanticAnnotation,
		VegetationCalibrationDomain::FunctionalTraits,
		VegetationCalibrationDomain::Hydraulics,
		VegetationCalibrationDomain::SizeAllometry,
		VegetationCalibrationDomain::SubstrateSuitability,
		VegetationCalibrationDomain::BuildingPlacement,
		VegetationCalibrationDomain::RepresentationFidelity,
		VegetationCalibrationDomain::RepresentationFidelity,
	};
	const VegetationCalibrationReadinessReport readiness =
		VegetationCalibrationReadinessEvaluationService().evaluate(
			calibrated_model, domains);
	assert(readiness.validationReport().passed());
	assert(readiness.observations().size() == 14u);
	assert(readiness.allRequestedDomainsReady());
	for (VegetationCalibrationDomain domain : domains) {
		assert(readiness.readyFor(domain));
	}

	const VegetationBiologicalProfile contradictory_profile(
		PlantIdentityProfile(
			PlantIdentityResolutionKind::ArchitecturalArchetype,
			PlantArchitecture::Shrub,
			std::string(),
			"Invented taxon"),
		PlantArchitecturalTopologyProfile(
			VegetationObjectCompositionKind::IndividualPlant,
			{PlantArchitecturalScale::WholePlant,
			 PlantArchitecturalScale::WholePlant},
			{PlantTopologyRelationshipKind::Branching,
			 PlantTopologyRelationshipKind::Branching},
			true,
			false),
		PlantPhenologyProfile(
			PlantDevelopmentState::Mature,
			{PlantDevelopmentState::Dormant,
			 PlantDevelopmentState::Dormant}),
		PlantRootArchitectureProfile(
			PlantRootRepresentationKind::ExplicitRootGraph,
			1.0,
			std::nullopt,
			0u),
		PlantCanopyOpticalProfile(2.0, 1.2, 45.0),
		PlantBiomechanicalProfile(600.0, -1.0, 0.03, 0.8),
		PlantEnvironmentalResponseProfile({
			PlantEnvironmentalDriver::Light,
			PlantEnvironmentalDriver::Light,
		}),
		VegetationEvidenceProfile(
			std::string(), std::string(), std::string()),
		PlantSemanticAnnotationProfile(
			"PlantOntology",
			std::string(),
			{"PO:0000003", "PO:0000003"},
			{},
			{"ontology-crosswalk:contradictory"}),
		PlantFunctionalTraitProfile(-1.0, 1.2, 0.0, 0.0),
		PlantHydraulicProfile(0.0, -1.0, 2.0, 1.0, 0.0),
		PlantSizeAllometryProfile(
			0.0, -1.0, -0.5, std::string()),
		VegetationSubstrateRequirementProfile(
			0.0, -1.0, 0.0, 1.4, 0.0));
	const BuildingVegetationObjectModel contradictory_model(
		"BVO.Contradictory",
		"Contradictory",
		PlantArchitecture::Tree,
		calibrated_spatial_profile(),
		VegetationRepresentationFidelityProfile(
			95.0, 95.0, 95.0, 95.0, 95.0, 95.0),
		VegetationTriangleDistributionPolicy(6000u),
		contradictory_profile);
	const VegetationBiologicalProfileValidationReport contradictory_validation =
		VegetationBiologicalProfileValidationService().validate(
			contradictory_model);
	assert(!contradictory_validation.passed());
	for (VegetationBiologicalProfileValidationCode expected_code : {
	     VegetationBiologicalProfileValidationCode::ArchitectureMismatch,
	     VegetationBiologicalProfileValidationCode::EmptyArchitecturalArchetype,
	     VegetationBiologicalProfileValidationCode::ContradictoryIdentityResolution,
	     VegetationBiologicalProfileValidationCode::DuplicateArchitecturalScale,
	     VegetationBiologicalProfileValidationCode::MissingArchitecturalScale,
	     VegetationBiologicalProfileValidationCode::DuplicateTopologyRelationship,
	     VegetationBiologicalProfileValidationCode::MissingTopologyRelationship,
	     VegetationBiologicalProfileValidationCode::MissingShootSystem,
	     VegetationBiologicalProfileValidationCode::DuplicatePhenologyState,
	     VegetationBiologicalProfileValidationCode::UnsupportedReferencePhenologyState,
	     VegetationBiologicalProfileValidationCode::IncompleteRootCalibration,
	     VegetationBiologicalProfileValidationCode::InvalidCanopyOpticalCalibration,
	     VegetationBiologicalProfileValidationCode::InvalidBiomechanicalCalibration,
	     VegetationBiologicalProfileValidationCode::IncompleteSemanticAnnotationCalibration,
	     VegetationBiologicalProfileValidationCode::DuplicateSemanticAnnotation,
	     VegetationBiologicalProfileValidationCode::InvalidFunctionalTraitCalibration,
	     VegetationBiologicalProfileValidationCode::InvalidHydraulicCalibration,
	     VegetationBiologicalProfileValidationCode::InvalidSizeAllometryCalibration,
	     VegetationBiologicalProfileValidationCode::InvalidSubstrateRequirementCalibration,
	     VegetationBiologicalProfileValidationCode::DuplicateEnvironmentalDriver,
	     VegetationBiologicalProfileValidationCode::EmptyGeometryAuthority,
	     VegetationBiologicalProfileValidationCode::EmptyBiologicalAuthority,
	     VegetationBiologicalProfileValidationCode::EmptyUncertaintyStatement}) {
		assert(has_biological_validation_issue(
			contradictory_validation, expected_code));
	}
	const VegetationCalibrationReadinessReport contradictory_readiness =
		VegetationCalibrationReadinessEvaluationService().evaluate(
			contradictory_model,
			{VegetationCalibrationDomain::BuildingPlacement});
	assert(!contradictory_readiness.allRequestedDomainsReady());
	assert(!contradictory_readiness.readyFor(
		VegetationCalibrationDomain::BuildingPlacement));
}

void verify_building_vegetation_object_composition()
{
	const BuildingObjectSpatialProfile spatial_profile(
		SpatialObjectId("SMB_VEGETATION_COURTYARD_TREE"),
		BuildingSpatialManifestationKind::PhysicalAggregate,
		BuildingPlacementPolicyKind::ConstraintSolved,
		BuildingCollisionBehaviorKind::SoftBoundary,
		BuildingObjectExtentSet(),
		BuildingOrientationProfile(
			BuildingOrientationPolicyKind::KeepUpright,
			BuildingRotationalSymmetryKind::ContinuousYaw,
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)),
		{}, {"landscape_support_graph", "vegetation_growth_graph"},
		1u, 0u);
	const BuildingVegetationObjectModel model(
		"BVO.CourtyardTree", "Courtyard Tree", PlantArchitecture::Tree,
		spatial_profile,
		VegetationRepresentationFidelityProfile(
			95.0, 95.0, 95.0, 95.0, 95.0, 95.0),
		VegetationTriangleDistributionPolicy(6000u));
	assert(model.modelIdentifier() == "BVO.CourtyardTree");
	assert(model.plantArchitecture() == PlantArchitecture::Tree);
	assert(model.spatialProfile().isSpatiallyApplicable());
	assert(model.fidelityProfile().minimumSilhouetteMatchPercent() == 95.0);
	assert(model.triangleDistributionPolicy().targetTriangleCount() == 6000u);
	assert(model.biologicalProfile().identity().architecturalArchetypeName() ==
	       "Tree");
	assert(!model.biologicalProfile().identity().isTaxonomicallyCalibrated());
	assert(model.biologicalProfile().architecturalTopology().includesRootSystem());
	assert(model.biologicalProfile().architecturalTopology().scales().size() == 7u);
	assert(model.biologicalProfile().phenology().referenceState() ==
	       PlantDevelopmentState::Mature);
	assert(!model.biologicalProfile().phenology().hasCalibratedSeasonalSchedule());
	assert(model.biologicalProfile().rootArchitecture().representation() ==
	       PlantRootRepresentationKind::SafetyEnvelope);
	assert(!model.biologicalProfile().rootArchitecture()
	            .hasCalibratedRootArchitecture());
	assert(!model.biologicalProfile().canopyOptics()
	            .supportsCalibratedLightInterception());
	assert(!model.biologicalProfile().biomechanics()
	            .supportsCalibratedWindSimulation());
	assert(!model.biologicalProfile().environmentalResponse()
	            .hasCalibratedResponses());
	assert(!model.biologicalProfile().semanticAnnotations()
	            .hasCalibratedSemanticAnnotations());
	assert(!model.biologicalProfile().functionalTraits()
	            .supportsCalibratedFunctionalTraitInference());
	assert(!model.biologicalProfile().hydraulics()
	            .supportsCalibratedWaterTransport());
	assert(!model.biologicalProfile().sizeAllometry()
	            .supportsCalibratedSizeProjection());
	assert(!model.biologicalProfile().substrateRequirements()
	            .supportsCalibratedSubstrateSuitability());
	assert(model.biologicalProfile().evidence().sourceIdentifiers().empty());
}

void verify_three_view_fidelity_gate()
{
	const VegetationRepresentationFidelityProfile profile(
		95.0, 95.0, 95.0, 95.0, 95.0, 95.0);
	const VegetationRepresentationFidelityEvaluationService service;
	const VegetationRepresentationFidelityReport passing = service.evaluate(
		profile,
		{
			VegetationViewFidelityObservation(
				VegetationReferenceView::Front,
				97.0, 96.0, 95.5, 98.0, 96.0, 95.2),
			VegetationViewFidelityObservation(
				VegetationReferenceView::Right,
				96.0, 95.5, 96.0, 97.0, 95.4, 96.0),
			VegetationViewFidelityObservation(
				VegetationReferenceView::Top,
				98.0, 97.0, 96.0, 95.5, 97.0, 95.1)});
	assert(passing.passed());

	const VegetationRepresentationFidelityReport failing = service.evaluate(
		profile,
		{
			VegetationViewFidelityObservation(
				VegetationReferenceView::Front,
				97.0, 96.0, 95.5, 98.0, 96.0, 95.2),
			VegetationViewFidelityObservation(
				VegetationReferenceView::Right,
				96.0, 95.5, 91.0, 97.0, 95.4, 96.0),
			VegetationViewFidelityObservation(
				VegetationReferenceView::Top,
				98.0, 94.0, 96.0, 95.5, 97.0, 95.1)});
	assert(!failing.passed());
	assert(failing.issues().size() == 2u);
	assert(failing.issues()[0].metricName() == "CrownGapFraction");
	assert(failing.issues()[1].metricName() == "ProjectedFoliageArea");

	const VegetationRepresentationFidelityReport missing_view = service.evaluate(
		profile,
		{VegetationViewFidelityObservation(
			 VegetationReferenceView::Front,
			 97.0, 96.0, 95.5, 98.0, 96.0, 95.2)});
	assert(!missing_view.passed());
	assert(!missing_view.diagnostic().empty());
}

} // namespace

int main()
{
	verify_exact_deterministic_distribution();
	verify_full_resolution_distribution();
	verify_fail_closed_requests();
	verify_biological_profile_validation_and_readiness();
	verify_building_vegetation_object_composition();
	verify_three_view_fidelity_gate();
	std::cout << "Building vegetation triangle distribution checks passed.\n";
	return 0;
}
