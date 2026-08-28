#include "vegetation/service/VegetationBiologicalProfileValidationService.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace {

template <typename Value>
bool contains(const std::vector<Value> &values, Value expected)
{
	return std::find(values.begin(), values.end(), expected) != values.end();
}

template <typename Value>
bool contains_duplicates(const std::vector<Value> &values)
{
	return std::set<Value>(values.begin(), values.end()).size() != values.size();
}

bool finite_non_negative(const std::optional<double> &value)
{
	return !value.has_value() ||
	       (std::isfinite(value.value()) && value.value() >= 0.0);
}

bool finite_positive(const std::optional<double> &value)
{
	return !value.has_value() ||
	       (std::isfinite(value.value()) && value.value() > 0.0);
}

bool has_any_root_calibration(const PlantRootArchitectureProfile &profile)
{
	return profile.representation() == PlantRootRepresentationKind::ExplicitRootGraph ||
	       profile.maximumDepthMetres().has_value() ||
	       profile.maximumRadialSpreadMetres().has_value() ||
	       profile.maximumRootOrder().has_value() ||
	       !profile.rootGraphIdentifier().empty() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_canopy_calibration(const PlantCanopyOpticalProfile &profile)
{
	return profile.leafAreaIndex().has_value() ||
	       profile.crownGapFraction().has_value() ||
	       profile.meanLeafInclinationDegrees().has_value() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_biomechanical_calibration(
	const PlantBiomechanicalProfile &profile)
{
	return profile.massDensityKilogramsPerCubicMetre().has_value() ||
	       profile.elasticModulusPascals().has_value() ||
	       profile.dampingRatio().has_value() ||
	       profile.dragCoefficient().has_value() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_semantic_annotation_calibration(
	const PlantSemanticAnnotationProfile &profile)
{
	return !profile.ontologyIdentifier().empty() ||
	       !profile.ontologyReleaseIdentifier().empty() ||
	       !profile.anatomicalEntityIdentifiers().empty() ||
	       !profile.developmentStageIdentifiers().empty() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_functional_trait_calibration(
	const PlantFunctionalTraitProfile &profile)
{
	return profile.specificLeafAreaSquareMetresPerKilogram().has_value() ||
	       profile.leafDryMatterContentKilogramsPerKilogram().has_value() ||
	       profile.leafNitrogenContentKilogramsPerKilogram().has_value() ||
	       profile.maximumMatureHeightMetres().has_value() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_hydraulic_calibration(const PlantHydraulicProfile &profile)
{
	return profile
		       .maximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal()
		       .has_value() ||
	       profile.hydraulicCapacitanceKilogramsPerMegapascal().has_value() ||
	       profile
		       .xylemWaterPotentialAtFiftyPercentConductivityLossMegapascals()
		       .has_value() ||
	       profile
		       .stomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals()
		       .has_value() ||
	       profile.leafToSapwoodAreaRatio().has_value() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_size_allometry_calibration(
	const PlantSizeAllometryProfile &profile)
{
	return profile.referenceHeightMetres().has_value() ||
	       profile.referenceHorizontalRadiusMetres().has_value() ||
	       profile.referenceSupportingAxisDiameterMetres().has_value() ||
	       !profile.sizeRelationshipModelIdentifier().empty() ||
	       !profile.evidenceIdentifiers().empty();
}

bool has_any_substrate_requirement_calibration(
	const VegetationSubstrateRequirementProfile &profile)
{
	return profile.minimumRootableVolumeCubicMetres().has_value() ||
	       profile.minimumRootableDepthMetres().has_value() ||
	       profile.maximumBulkDensityKilogramsPerCubicMetre().has_value() ||
	       profile.minimumAirFilledPorosityFraction().has_value() ||
	       profile.minimumAvailableWaterCapacityFraction().has_value() ||
	       !profile.evidenceIdentifiers().empty();
}

bool finite_negative(const std::optional<double> &value)
{
	return !value.has_value() ||
	       (std::isfinite(value.value()) && value.value() < 0.0);
}

bool finite_fraction_above_zero(const std::optional<double> &value)
{
	return !value.has_value() ||
	       (std::isfinite(value.value()) && value.value() > 0.0 &&
		value.value() <= 1.0);
}

}

VegetationBiologicalProfileValidationReport
VegetationBiologicalProfileValidationService::validate(
	const BuildingVegetationObjectModel &object_model) const
{
	VegetationBiologicalProfileValidationReport report;
	if (object_model.modelIdentifier().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::EmptyModelIdentifier,
			"Building vegetation object model has an empty identifier.");
	}

	const VegetationBiologicalProfile &biological_profile =
		object_model.biologicalProfile();
	const PlantIdentityProfile &identity = biological_profile.identity();
	if (identity.architecture() != object_model.plantArchitecture()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::ArchitectureMismatch,
			"Plant identity architecture differs from the building vegetation object architecture.");
	}
	if (identity.architecturalArchetypeName().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::EmptyArchitecturalArchetype,
			"Plant identity has an empty architectural archetype name.");
	}
	if (identity.resolution() ==
		    PlantIdentityResolutionKind::ArchitecturalArchetype &&
	    (!identity.scientificName().empty() || !identity.cultivarName().empty() ||
	     !identity.specimenIdentifier().empty())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::ContradictoryIdentityResolution,
			"Architectural-archetype identity may not claim a scientific name, cultivar, or specimen.");
	}
	if (identity.resolution() !=
		    PlantIdentityResolutionKind::ArchitecturalArchetype &&
	    identity.scientificName().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingScientificName,
			"Taxon, cultivar, and measured-specimen identities require a scientific name.");
	}
	if (identity.resolution() == PlantIdentityResolutionKind::Cultivar &&
	    identity.cultivarName().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingCultivarName,
			"Cultivar identity requires a cultivar name.");
	}
	if (identity.resolution() == PlantIdentityResolutionKind::MeasuredSpecimen &&
	    identity.specimenIdentifier().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingSpecimenIdentifier,
			"Measured-specimen identity requires a specimen identifier.");
	}
	if (identity.resolution() !=
		    PlantIdentityResolutionKind::ArchitecturalArchetype &&
	    identity.evidenceIdentifiers().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingIdentityEvidence,
			"Resolved plant identity requires traceable identity evidence.");
	}

	const PlantArchitecturalTopologyProfile &topology =
		biological_profile.architecturalTopology();
	if (contains_duplicates(topology.scales())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::DuplicateArchitecturalScale,
			"Plant architectural topology declares a scale more than once.");
	}
	for (PlantArchitecturalScale required_scale : {
	     PlantArchitecturalScale::WholePlant,
	     PlantArchitecturalScale::Axis,
	     PlantArchitecturalScale::Organ}) {
		if (!contains(topology.scales(), required_scale)) {
			report.addIssue(
				VegetationBiologicalProfileValidationCode::MissingArchitecturalScale,
				"Plant architectural topology is missing a required biological scale.");
		}
	}
	if (contains_duplicates(topology.relationshipKinds())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::DuplicateTopologyRelationship,
			"Plant architectural topology declares a relationship kind more than once.");
	}
	for (PlantTopologyRelationshipKind required_relationship : {
	     PlantTopologyRelationshipKind::Decomposition,
	     PlantTopologyRelationshipKind::Succession,
	     PlantTopologyRelationshipKind::Branching,
	     PlantTopologyRelationshipKind::Attachment}) {
		if (!contains(topology.relationshipKinds(), required_relationship)) {
			report.addIssue(
				VegetationBiologicalProfileValidationCode::MissingTopologyRelationship,
				"Plant architectural topology is missing a required relationship kind.");
		}
	}
	if (!topology.includesShootSystem()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingShootSystem,
			"Building vegetation requires an explicit shoot-system topology.");
	}
	if ((!topology.topologyEvidenceIdentifiers().empty() ||
	     topology.maximumBranchOrder().has_value()) &&
	    !topology.hasCalibratedShootTopology()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::IncompleteShootTopologyCalibration,
			"Shoot-topology calibration requires positive maximum branch order and evidence.");
	}

	const PlantPhenologyProfile &phenology = biological_profile.phenology();
	if (contains_duplicates(phenology.supportedStates())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::DuplicatePhenologyState,
			"Plant phenology declares a developmental state more than once.");
	}
	if (!contains(phenology.supportedStates(), phenology.referenceState())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::UnsupportedReferencePhenologyState,
			"Plant phenology reference state is not included in supported states.");
	}

	const PlantRootArchitectureProfile &root =
		biological_profile.rootArchitecture();
	if (!finite_positive(root.maximumDepthMetres()) ||
	    !finite_positive(root.maximumRadialSpreadMetres()) ||
	    (root.maximumRootOrder().has_value() &&
	     root.maximumRootOrder().value() == 0u) ||
	    (has_any_root_calibration(root) &&
	     !root.hasCalibratedRootArchitecture())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::IncompleteRootCalibration,
			"Root calibration requires an explicit graph, positive depth and spread, positive order, and evidence.");
	}

	const PlantCanopyOpticalProfile &canopy =
		biological_profile.canopyOptics();
	if (!finite_non_negative(canopy.leafAreaIndex()) ||
	    !finite_non_negative(canopy.crownGapFraction()) ||
	    (canopy.crownGapFraction().has_value() &&
	     canopy.crownGapFraction().value() > 1.0) ||
	    !finite_non_negative(canopy.meanLeafInclinationDegrees()) ||
	    (canopy.meanLeafInclinationDegrees().has_value() &&
	     canopy.meanLeafInclinationDegrees().value() > 90.0) ||
	    (has_any_canopy_calibration(canopy) &&
	     !canopy.supportsCalibratedLightInterception())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidCanopyOpticalCalibration,
			"Canopy calibration requires finite leaf area, gap fraction, leaf inclination, and evidence within valid ranges.");
	}

	const PlantBiomechanicalProfile &biomechanics =
		biological_profile.biomechanics();
	if (!finite_positive(
		    biomechanics.massDensityKilogramsPerCubicMetre()) ||
	    !finite_positive(biomechanics.elasticModulusPascals()) ||
	    !finite_non_negative(biomechanics.dampingRatio()) ||
	    !finite_positive(biomechanics.dragCoefficient()) ||
	    (has_any_biomechanical_calibration(biomechanics) &&
	     !biomechanics.supportsCalibratedWindSimulation())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidBiomechanicalCalibration,
			"Biomechanical calibration requires positive density, stiffness and drag, non-negative damping, and evidence.");
	}

	const PlantSemanticAnnotationProfile &semantic_annotations =
		biological_profile.semanticAnnotations();
	if (contains_duplicates(
		    semantic_annotations.anatomicalEntityIdentifiers()) ||
	    contains_duplicates(
		    semantic_annotations.developmentStageIdentifiers())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::DuplicateSemanticAnnotation,
			"Plant semantic annotations declare an ontology term more than once.");
	}
	if (has_any_semantic_annotation_calibration(semantic_annotations) &&
	    !semantic_annotations.hasCalibratedSemanticAnnotations()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::IncompleteSemanticAnnotationCalibration,
			"Semantic annotation calibration requires an ontology, release, anatomy terms, development terms, and evidence.");
	}

	const PlantFunctionalTraitProfile &functional_traits =
		biological_profile.functionalTraits();
	if (!finite_positive(
		    functional_traits.specificLeafAreaSquareMetresPerKilogram()) ||
	    !finite_fraction_above_zero(
		    functional_traits.leafDryMatterContentKilogramsPerKilogram()) ||
	    !finite_fraction_above_zero(
		    functional_traits.leafNitrogenContentKilogramsPerKilogram()) ||
	    !finite_positive(functional_traits.maximumMatureHeightMetres()) ||
	    (has_any_functional_trait_calibration(functional_traits) &&
	     !functional_traits.supportsCalibratedFunctionalTraitInference())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidFunctionalTraitCalibration,
			"Functional-trait calibration requires positive leaf area and mature height, bounded leaf fractions, and evidence.");
	}

	const PlantHydraulicProfile &hydraulics = biological_profile.hydraulics();
	if (!finite_positive(
		    hydraulics.maximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal()) ||
	    !finite_positive(
		    hydraulics.hydraulicCapacitanceKilogramsPerMegapascal()) ||
	    !finite_negative(
		    hydraulics.xylemWaterPotentialAtFiftyPercentConductivityLossMegapascals()) ||
	    !finite_negative(
		    hydraulics.stomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals()) ||
	    !finite_positive(hydraulics.leafToSapwoodAreaRatio()) ||
	    (has_any_hydraulic_calibration(hydraulics) &&
	     !hydraulics.supportsCalibratedWaterTransport())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidHydraulicCalibration,
			"Hydraulic calibration requires positive conductance, capacitance and area ratio, negative vulnerability potentials, and evidence.");
	}

	const PlantSizeAllometryProfile &size_allometry =
		biological_profile.sizeAllometry();
	if (!finite_positive(size_allometry.referenceHeightMetres()) ||
	    !finite_positive(size_allometry.referenceHorizontalRadiusMetres()) ||
	    !finite_positive(
		    size_allometry.referenceSupportingAxisDiameterMetres()) ||
	    (has_any_size_allometry_calibration(size_allometry) &&
	     !size_allometry.supportsCalibratedSizeProjection())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidSizeAllometryCalibration,
			"Size allometry requires positive reference dimensions, a relationship model, and evidence.");
	}

	const VegetationSubstrateRequirementProfile &substrate =
		biological_profile.substrateRequirements();
	if (!finite_positive(substrate.minimumRootableVolumeCubicMetres()) ||
	    !finite_positive(substrate.minimumRootableDepthMetres()) ||
	    !finite_positive(
		    substrate.maximumBulkDensityKilogramsPerCubicMetre()) ||
	    !finite_fraction_above_zero(
		    substrate.minimumAirFilledPorosityFraction()) ||
	    !finite_fraction_above_zero(
		    substrate.minimumAvailableWaterCapacityFraction()) ||
	    (has_any_substrate_requirement_calibration(substrate) &&
	     !substrate.supportsCalibratedSubstrateSuitability())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::InvalidSubstrateRequirementCalibration,
			"Substrate calibration requires rootable volume and depth, bulk-density and porosity limits, water capacity, and evidence.");
	}

	const PlantEnvironmentalResponseProfile &environment =
		biological_profile.environmentalResponse();
	if (environment.requiredDrivers().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::MissingEnvironmentalDriver,
			"Environmental response requires at least one explicit driver.");
	}
	if (contains_duplicates(environment.requiredDrivers())) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::DuplicateEnvironmentalDriver,
			"Environmental response declares a driver more than once.");
	}

	const VegetationEvidenceProfile &evidence = biological_profile.evidence();
	if (evidence.geometryAuthority().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::EmptyGeometryAuthority,
			"Vegetation evidence has no geometry authority.");
	}
	if (evidence.biologicalAuthority().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::EmptyBiologicalAuthority,
			"Vegetation evidence has no biological authority.");
	}
	if (evidence.uncertaintyStatement().empty()) {
		report.addIssue(
			VegetationBiologicalProfileValidationCode::EmptyUncertaintyStatement,
			"Vegetation evidence has no uncertainty statement.");
	}
	return report;
}
