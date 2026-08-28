#include "vegetation/service/VegetationCalibrationEvidenceBindingService.h"

#include "vegetation/service/VegetationCalibrationEvidenceBundleValidationService.h"
#include "vegetation/service/VegetationCalibrationReadinessEvaluationService.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using MeasurementKind = VegetationCalibrationMeasurementKind;
using BindingDisposition = VegetationCalibrationEvidenceBindingDisposition;

const std::vector<VegetationCalibrationDomain> &all_calibration_domains()
{
	static const std::vector<VegetationCalibrationDomain> domains = {
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
	};
	return domains;
}

std::vector<VegetationCalibrationDomain> ready_domains(
	const BuildingVegetationObjectModel &object_model)
{
	const VegetationCalibrationReadinessReport readiness =
		VegetationCalibrationReadinessEvaluationService().evaluate(
			object_model, all_calibration_domains());
	std::vector<VegetationCalibrationDomain> domains;
	for (VegetationCalibrationDomain domain : all_calibration_domains()) {
		if (readiness.readyFor(domain)) domains.push_back(domain);
	}
	return domains;
}

using MeasurementIndex = std::map<
	MeasurementKind,
	const VegetationCalibrationMeasurement *>;

MeasurementIndex measurement_index(
	const VegetationCalibrationEvidenceBundle &bundle)
{
	MeasurementIndex index;
	for (const VegetationCalibrationMeasurement &measurement :
	     bundle.measurements()) {
		index.emplace(measurement.kind(), &measurement);
	}
	return index;
}

bool contains_all(
	const MeasurementIndex &index,
	const std::vector<MeasurementKind> &required_kinds)
{
	return std::all_of(
		required_kinds.begin(), required_kinds.end(),
		[&index](MeasurementKind kind) {
			return index.find(kind) != index.end();
		});
}

bool contains_any(
	const MeasurementIndex &index,
	const std::vector<MeasurementKind> &required_kinds)
{
	return std::any_of(
		required_kinds.begin(), required_kinds.end(),
		[&index](MeasurementKind kind) {
			return index.find(kind) != index.end();
		});
}

const VegetationCalibrationMeasurement &measurement(
	const MeasurementIndex &index,
	MeasurementKind kind)
{
	return *index.at(kind);
}

double decimal_value(const MeasurementIndex &index, MeasurementKind kind)
{
	return measurement(index, kind).value().decimalValue().value();
}

std::size_t count_value(const MeasurementIndex &index, MeasurementKind kind)
{
	return measurement(index, kind).value().countValue().value();
}

std::string text_value(const MeasurementIndex &index, MeasurementKind kind)
{
	return *measurement(index, kind).value().textValue();
}

std::vector<std::string> identifier_list_value(
	const MeasurementIndex &index,
	MeasurementKind kind)
{
	return *measurement(index, kind).value().identifierListValue();
}

std::vector<std::string> unique_sorted(std::vector<std::string> values)
{
	std::sort(values.begin(), values.end());
	values.erase(std::unique(values.begin(), values.end()), values.end());
	return values;
}

std::vector<std::string> evidence_identifiers(
	const MeasurementIndex &index,
	const std::vector<MeasurementKind> &measurement_kinds,
	std::vector<std::string> additional_identifiers = {})
{
	for (MeasurementKind kind : measurement_kinds) {
		const VegetationCalibrationMeasurement &current = measurement(index, kind);
		additional_identifiers.insert(
			additional_identifiers.end(),
			current.evidenceSourceIdentifiers().begin(),
			current.evidenceSourceIdentifiers().end());
	}
	return unique_sorted(std::move(additional_identifiers));
}

PlantIdentityResolutionKind identity_resolution(
	VegetationCalibrationSubjectScopeKind scope_kind)
{
	switch (scope_kind) {
	case VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype:
		return PlantIdentityResolutionKind::ArchitecturalArchetype;
	case VegetationCalibrationSubjectScopeKind::BotanicalTaxon:
		return PlantIdentityResolutionKind::BotanicalTaxon;
	case VegetationCalibrationSubjectScopeKind::Cultivar:
		return PlantIdentityResolutionKind::Cultivar;
	case VegetationCalibrationSubjectScopeKind::MeasuredSpecimen:
		return PlantIdentityResolutionKind::MeasuredSpecimen;
	}
	return PlantIdentityResolutionKind::ArchitecturalArchetype;
}

std::vector<MeasurementKind> identity_requirements(
	const VegetationCalibrationSubjectScope &scope)
{
	std::vector<MeasurementKind> requirements;
	if (scope.kind() ==
	    VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype) {
		return requirements;
	}
	requirements.push_back(MeasurementKind::ScientificName);
	if (scope.kind() == VegetationCalibrationSubjectScopeKind::Cultivar ||
	    !scope.cultivarName().empty()) {
		requirements.push_back(MeasurementKind::CultivarName);
	}
	if (scope.kind() ==
	    VegetationCalibrationSubjectScopeKind::MeasuredSpecimen) {
		requirements.push_back(MeasurementKind::SpecimenIdentifier);
	}
	return requirements;
}

const std::map<VegetationCalibrationDomain, std::vector<MeasurementKind>> &
fixed_domain_requirements()
{
	static const std::map<
		VegetationCalibrationDomain,
		std::vector<MeasurementKind>> requirements = {
		{VegetationCalibrationDomain::ShootTopology,
		 {MeasurementKind::ShootTopologyArtifactIdentifier,
		  MeasurementKind::MaximumBranchOrder}},
		{VegetationCalibrationDomain::RootArchitecture,
		 {MeasurementKind::RootGraphIdentifier,
		  MeasurementKind::MaximumRootDepthMetres,
		  MeasurementKind::MaximumRootRadialSpreadMetres,
		  MeasurementKind::MaximumRootOrder}},
		{VegetationCalibrationDomain::CanopyOptics,
		 {MeasurementKind::LeafAreaIndex,
		  MeasurementKind::CrownGapFraction,
		  MeasurementKind::MeanLeafInclinationDegrees}},
		{VegetationCalibrationDomain::Phenology,
		 {MeasurementKind::SeasonalScheduleArtifactIdentifier}},
		{VegetationCalibrationDomain::Biomechanics,
		 {MeasurementKind::MassDensityKilogramsPerCubicMetre,
		  MeasurementKind::ElasticModulusPascals,
		  MeasurementKind::DampingRatio,
		  MeasurementKind::DragCoefficient}},
		{VegetationCalibrationDomain::EnvironmentalResponse,
		 {MeasurementKind::EnvironmentalResponseArtifactIdentifier}},
		{VegetationCalibrationDomain::SemanticAnnotation,
		 {MeasurementKind::OntologyIdentifier,
		  MeasurementKind::OntologyReleaseIdentifier,
		  MeasurementKind::AnatomicalEntityIdentifiers,
		  MeasurementKind::DevelopmentStageIdentifiers}},
		{VegetationCalibrationDomain::FunctionalTraits,
		 {MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram,
		  MeasurementKind::LeafDryMatterContentKilogramsPerKilogram,
		  MeasurementKind::LeafNitrogenContentKilogramsPerKilogram,
		  MeasurementKind::MaximumMatureHeightMetres}},
		{VegetationCalibrationDomain::Hydraulics,
		 {MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal,
		  MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal,
		  MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals,
		  MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals,
		  MeasurementKind::LeafToSapwoodAreaRatio}},
		{VegetationCalibrationDomain::SizeAllometry,
		 {MeasurementKind::ReferenceHeightMetres,
		  MeasurementKind::ReferenceHorizontalRadiusMetres,
		  MeasurementKind::ReferenceSupportingAxisDiameterMetres,
		  MeasurementKind::SizeRelationshipModelIdentifier}},
		{VegetationCalibrationDomain::SubstrateSuitability,
		 {MeasurementKind::MinimumRootableVolumeCubicMetres,
		  MeasurementKind::MinimumRootableDepthMetres,
		  MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre,
		  MeasurementKind::MinimumAirFilledPorosityFraction,
		  MeasurementKind::MinimumAvailableWaterCapacityFraction}},
		{VegetationCalibrationDomain::RepresentationFidelity,
		 {MeasurementKind::GrammarEvidenceIdentifier,
		  MeasurementKind::CameraEvidenceIdentifier}},
	};
	return requirements;
}

void add_domain_observations(
	const MeasurementIndex &index,
	const std::vector<MeasurementKind> &required_kinds,
	bool accepted,
	std::vector<VegetationCalibrationEvidenceBindingObservation> &observations)
{
	for (MeasurementKind kind : required_kinds) {
		if (index.find(kind) == index.end()) continue;
		observations.emplace_back(
			kind,
			accepted ? BindingDisposition::Accepted : BindingDisposition::Deferred,
			accepted
				? "Measurement was bound as part of a complete calibration domain."
				: "Measurement was deferred because its calibration domain is incomplete.");
	}
}

}

VegetationCalibrationEvidenceBindingReport
VegetationCalibrationEvidenceBindingService::bind(
	const BuildingVegetationObjectModel &object_model,
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	VegetationCalibrationEvidenceBundleValidationReport validation_report =
		VegetationCalibrationEvidenceBundleValidationService().validate(
			object_model, bundle);
	const std::vector<VegetationCalibrationDomain> ready_before =
		ready_domains(object_model);
	if (!validation_report.passed()) {
		std::vector<VegetationCalibrationEvidenceBindingObservation> observations;
		for (const VegetationCalibrationMeasurement &measurement :
		     bundle.measurements()) {
			observations.emplace_back(
				measurement.kind(), BindingDisposition::Rejected,
				"Measurement was rejected because the evidence bundle is invalid.");
		}
		return VegetationCalibrationEvidenceBindingReport(
			std::move(validation_report),
			std::move(observations),
			ready_before,
			ready_before,
			std::nullopt);
	}

	const MeasurementIndex index = measurement_index(bundle);
	const VegetationBiologicalProfile &source = object_model.biologicalProfile();
	PlantIdentityProfile identity = source.identity();
	PlantArchitecturalTopologyProfile topology = source.architecturalTopology();
	PlantPhenologyProfile phenology = source.phenology();
	PlantRootArchitectureProfile root = source.rootArchitecture();
	PlantCanopyOpticalProfile canopy = source.canopyOptics();
	PlantBiomechanicalProfile biomechanics = source.biomechanics();
	PlantEnvironmentalResponseProfile environment = source.environmentalResponse();
	PlantSemanticAnnotationProfile semantic = source.semanticAnnotations();
	PlantFunctionalTraitProfile traits = source.functionalTraits();
	PlantHydraulicProfile hydraulics = source.hydraulics();
	PlantSizeAllometryProfile allometry = source.sizeAllometry();
	VegetationSubstrateRequirementProfile substrate =
		source.substrateRequirements();
	std::vector<VegetationCalibrationEvidenceBindingObservation> observations;
	std::set<VegetationCalibrationDomain> accepted_domains;

	const std::vector<MeasurementKind> identity_kinds =
		identity_requirements(bundle.subjectScope());
	if (contains_any(index, identity_kinds)) {
		const bool accepted = contains_all(index, identity_kinds);
		add_domain_observations(index, identity_kinds, accepted, observations);
		if (accepted) {
			identity = PlantIdentityProfile(
				identity_resolution(bundle.subjectScope().kind()),
				object_model.plantArchitecture(),
				identity.architecturalArchetypeName(),
				bundle.subjectScope().scientificName(),
				bundle.subjectScope().cultivarName(),
				evidence_identifiers(index, identity_kinds),
				bundle.subjectScope().specimenIdentifier());
			accepted_domains.insert(
				VegetationCalibrationDomain::TaxonomicIdentity);
		}
	}

	for (const auto &domain_requirements : fixed_domain_requirements()) {
		const VegetationCalibrationDomain domain = domain_requirements.first;
		const std::vector<MeasurementKind> &kinds = domain_requirements.second;
		if (!contains_any(index, kinds)) continue;
		const bool accepted = contains_all(index, kinds);
		add_domain_observations(index, kinds, accepted, observations);
		if (!accepted) continue;
		accepted_domains.insert(domain);

		switch (domain) {
		case VegetationCalibrationDomain::ShootTopology: {
			const std::string artifact = text_value(
				index, MeasurementKind::ShootTopologyArtifactIdentifier);
			topology = PlantArchitecturalTopologyProfile(
				topology.composition(),
				topology.scales(),
				topology.relationshipKinds(),
				topology.includesRootSystem(),
				topology.includesShootSystem(),
				evidence_identifiers(index, kinds, {artifact}),
				count_value(index, MeasurementKind::MaximumBranchOrder));
			break;
		}
		case VegetationCalibrationDomain::RootArchitecture: {
			const std::string graph_identifier =
				text_value(index, MeasurementKind::RootGraphIdentifier);
			root = PlantRootArchitectureProfile(
				PlantRootRepresentationKind::ExplicitRootGraph,
				decimal_value(index, MeasurementKind::MaximumRootDepthMetres),
				decimal_value(
					index, MeasurementKind::MaximumRootRadialSpreadMetres),
				count_value(index, MeasurementKind::MaximumRootOrder),
				evidence_identifiers(index, kinds, {graph_identifier}),
				graph_identifier);
			break;
		}
		case VegetationCalibrationDomain::CanopyOptics:
			canopy = PlantCanopyOpticalProfile(
				decimal_value(index, MeasurementKind::LeafAreaIndex),
				decimal_value(index, MeasurementKind::CrownGapFraction),
				decimal_value(
					index, MeasurementKind::MeanLeafInclinationDegrees),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::Phenology: {
			const std::string artifact = text_value(
				index, MeasurementKind::SeasonalScheduleArtifactIdentifier);
			phenology = PlantPhenologyProfile(
				phenology.referenceState(),
				phenology.supportedStates(),
				evidence_identifiers(index, kinds, {artifact}));
			break;
		}
		case VegetationCalibrationDomain::Biomechanics:
			biomechanics = PlantBiomechanicalProfile(
				decimal_value(
					index, MeasurementKind::MassDensityKilogramsPerCubicMetre),
				decimal_value(index, MeasurementKind::ElasticModulusPascals),
				decimal_value(index, MeasurementKind::DampingRatio),
				decimal_value(index, MeasurementKind::DragCoefficient),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::EnvironmentalResponse: {
			const std::string artifact = text_value(
				index, MeasurementKind::EnvironmentalResponseArtifactIdentifier);
			environment = PlantEnvironmentalResponseProfile(
				environment.requiredDrivers(),
				evidence_identifiers(index, kinds, {artifact}));
			break;
		}
		case VegetationCalibrationDomain::SemanticAnnotation:
			semantic = PlantSemanticAnnotationProfile(
				text_value(index, MeasurementKind::OntologyIdentifier),
				text_value(index, MeasurementKind::OntologyReleaseIdentifier),
				identifier_list_value(
					index, MeasurementKind::AnatomicalEntityIdentifiers),
				identifier_list_value(
					index, MeasurementKind::DevelopmentStageIdentifiers),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::FunctionalTraits:
			traits = PlantFunctionalTraitProfile(
				decimal_value(
					index,
					MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram),
				decimal_value(
					index,
					MeasurementKind::LeafDryMatterContentKilogramsPerKilogram),
				decimal_value(
					index,
					MeasurementKind::LeafNitrogenContentKilogramsPerKilogram),
				decimal_value(index, MeasurementKind::MaximumMatureHeightMetres),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::Hydraulics:
			hydraulics = PlantHydraulicProfile(
				decimal_value(
					index,
					MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal),
				decimal_value(
					index,
					MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal),
				decimal_value(
					index,
					MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals),
				decimal_value(
					index,
					MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals),
				decimal_value(index, MeasurementKind::LeafToSapwoodAreaRatio),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::SizeAllometry:
			allometry = PlantSizeAllometryProfile(
				decimal_value(index, MeasurementKind::ReferenceHeightMetres),
				decimal_value(
					index, MeasurementKind::ReferenceHorizontalRadiusMetres),
				decimal_value(
					index,
					MeasurementKind::ReferenceSupportingAxisDiameterMetres),
				text_value(
					index, MeasurementKind::SizeRelationshipModelIdentifier),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::SubstrateSuitability:
			substrate = VegetationSubstrateRequirementProfile(
				decimal_value(
					index, MeasurementKind::MinimumRootableVolumeCubicMetres),
				decimal_value(
					index, MeasurementKind::MinimumRootableDepthMetres),
				decimal_value(
					index,
					MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre),
				decimal_value(
					index, MeasurementKind::MinimumAirFilledPorosityFraction),
				decimal_value(
					index,
					MeasurementKind::MinimumAvailableWaterCapacityFraction),
				evidence_identifiers(index, kinds));
			break;
		case VegetationCalibrationDomain::RepresentationFidelity:
		case VegetationCalibrationDomain::TaxonomicIdentity:
		case VegetationCalibrationDomain::BuildingPlacement:
			break;
		}
	}

	VegetationEvidenceProfile evidence = source.evidence();
	if (!accepted_domains.empty()) {
		std::vector<std::string> source_identifiers = evidence.sourceIdentifiers();
		source_identifiers.push_back(bundle.bundleIdentifier());
		for (const VegetationCalibrationEvidenceBindingObservation &observation :
		     observations) {
			if (observation.disposition() != BindingDisposition::Accepted) continue;
			const VegetationCalibrationMeasurement &accepted =
				measurement(index, observation.measurementKind());
			source_identifiers.insert(
				source_identifiers.end(),
				accepted.evidenceSourceIdentifiers().begin(),
				accepted.evidenceSourceIdentifiers().end());
		}
		evidence = VegetationEvidenceProfile(
			evidence.geometryAuthority(),
			"CalibrationEvidenceBundle:" + bundle.bundleIdentifier(),
			evidence.uncertaintyStatement() + " " +
				bundle.uncertaintyStatement(),
			unique_sorted(std::move(source_identifiers)));
	}

	VegetationBiologicalProfile bound_profile(
		std::move(identity),
		std::move(topology),
		std::move(phenology),
		std::move(root),
		std::move(canopy),
		std::move(biomechanics),
		std::move(environment),
		std::move(evidence),
		std::move(semantic),
		std::move(traits),
		std::move(hydraulics),
		std::move(allometry),
		std::move(substrate));
	const BuildingVegetationObjectModel bound_model(
		object_model.modelIdentifier(),
		object_model.displayName(),
		object_model.plantArchitecture(),
		object_model.spatialProfile(),
		object_model.fidelityProfile(),
		object_model.triangleDistributionPolicy(),
		bound_profile);
	return VegetationCalibrationEvidenceBindingReport(
		std::move(validation_report),
		std::move(observations),
		ready_before,
		ready_domains(bound_model),
		std::move(bound_profile));
}
