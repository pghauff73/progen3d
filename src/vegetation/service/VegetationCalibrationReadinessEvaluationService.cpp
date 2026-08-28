#include "vegetation/service/VegetationCalibrationReadinessEvaluationService.h"

#include "building/model/BuildingExtentKind.h"
#include "vegetation/service/VegetationBiologicalProfileValidationService.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>

namespace {

bool has_graph_membership(
	const BuildingObjectSpatialProfile &spatial_profile,
	const std::string &graph_identifier)
{
	return std::find(
		       spatial_profile.graphMemberships().begin(),
		       spatial_profile.graphMemberships().end(),
		       graph_identifier) != spatial_profile.graphMemberships().end();
}

bool has_authored_extent(
	const BuildingObjectSpatialProfile &spatial_profile,
	BuildingExtentKind extent_kind)
{
	const BuildingExtentDescriptor *extent =
		spatial_profile.extentSet().find(extent_kind);
	return extent != nullptr &&
	       extent->applicability() != BuildingExtentApplicability::NotApplicable &&
	       extent->representationKind().has_value() &&
	       extent->sourceKind() != BuildingExtentSourceKind::None &&
	       !extent->provenance().empty();
}

bool valid_fidelity_threshold(double threshold)
{
	return std::isfinite(threshold) && threshold >= 0.0 && threshold <= 100.0;
}

std::vector<std::string> unmet_requirements_for(
	const BuildingVegetationObjectModel &object_model,
	VegetationCalibrationDomain domain)
{
	const VegetationBiologicalProfile &biological_profile =
		object_model.biologicalProfile();
	std::vector<std::string> unmet_requirements;
	switch (domain) {
	case VegetationCalibrationDomain::TaxonomicIdentity:
		if (!biological_profile.identity().isTaxonomicallyCalibrated()) {
			unmet_requirements.push_back(
				"Scientific identity and traceable identity evidence are required.");
		}
		break;
	case VegetationCalibrationDomain::ShootTopology:
		if (!biological_profile.architecturalTopology()
		         .hasCalibratedShootTopology()) {
			unmet_requirements.push_back(
				"Shoot topology requires a multiscale topology evidence identifier.");
		}
		break;
	case VegetationCalibrationDomain::RootArchitecture:
		if (!biological_profile.rootArchitecture()
		         .hasCalibratedRootArchitecture()) {
			unmet_requirements.push_back(
				"Root readiness requires an explicit root graph, dimensions, order, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::CanopyOptics:
		if (!biological_profile.canopyOptics()
		         .supportsCalibratedLightInterception()) {
			unmet_requirements.push_back(
				"Canopy readiness requires leaf area, gap fraction, leaf inclination, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::Phenology:
		if (!biological_profile.phenology().hasCalibratedSeasonalSchedule()) {
			unmet_requirements.push_back(
				"Phenology readiness requires a traceable seasonal schedule.");
		}
		break;
	case VegetationCalibrationDomain::Biomechanics:
		if (!biological_profile.biomechanics()
		         .supportsCalibratedWindSimulation()) {
			unmet_requirements.push_back(
				"Biomechanical readiness requires density, stiffness, damping, drag, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::EnvironmentalResponse:
		if (!biological_profile.environmentalResponse()
		         .hasCalibratedResponses()) {
			unmet_requirements.push_back(
				"Environmental response readiness requires calibrated response evidence.");
		}
		break;
	case VegetationCalibrationDomain::SemanticAnnotation:
		if (!biological_profile.semanticAnnotations()
		         .hasCalibratedSemanticAnnotations()) {
			unmet_requirements.push_back(
				"Semantic annotation readiness requires versioned anatomy and development ontology terms with evidence.");
		}
		break;
	case VegetationCalibrationDomain::FunctionalTraits:
		if (!biological_profile.functionalTraits()
		         .supportsCalibratedFunctionalTraitInference()) {
			unmet_requirements.push_back(
				"Functional-trait readiness requires leaf traits, mature height, and observation evidence.");
		}
		break;
	case VegetationCalibrationDomain::Hydraulics:
		if (!biological_profile.hydraulics()
		         .supportsCalibratedWaterTransport()) {
			unmet_requirements.push_back(
				"Hydraulic readiness requires conductance, capacitance, vulnerability thresholds, area ratio, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::SizeAllometry:
		if (!biological_profile.sizeAllometry()
		         .supportsCalibratedSizeProjection()) {
			unmet_requirements.push_back(
				"Size-allometry readiness requires reference dimensions, a relationship model, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::SubstrateSuitability:
		if (!biological_profile.substrateRequirements()
		         .supportsCalibratedSubstrateSuitability()) {
			unmet_requirements.push_back(
				"Substrate readiness requires rootable volume, depth, density, porosity, water capacity, and evidence.");
		}
		break;
	case VegetationCalibrationDomain::BuildingPlacement: {
		const BuildingObjectSpatialProfile &spatial_profile =
			object_model.spatialProfile();
		if (!spatial_profile.isSpatiallyApplicable()) {
			unmet_requirements.push_back(
				"Building placement requires a spatially applicable object profile.");
		}
		if (spatial_profile.placementPolicy() ==
		    BuildingPlacementPolicyKind::NotApplicable) {
			unmet_requirements.push_back(
				"Building placement requires an explicit placement policy.");
		}
		if (spatial_profile.collisionBehavior() ==
		    BuildingCollisionBehaviorKind::NonParticipating) {
			unmet_requirements.push_back(
				"Building placement requires collision participation.");
		}
		if (!spatial_profile.orientationProfile().isApplicable()) {
			unmet_requirements.push_back(
				"Building placement requires an applicable orientation profile.");
		}
		for (BuildingExtentKind extent_kind : {
		     BuildingExtentKind::Visual,
		     BuildingExtentKind::Collision,
		     BuildingExtentKind::Growth}) {
			if (!has_authored_extent(spatial_profile, extent_kind)) {
				unmet_requirements.push_back(
					"Building placement requires visual, collision, and growth extents with provenance.");
				break;
			}
		}
		for (const std::string &graph_identifier : {
		     std::string("landscape_support_graph"),
		     std::string("vegetation_growth_graph")}) {
			if (!has_graph_membership(spatial_profile, graph_identifier)) {
				unmet_requirements.push_back(
					"Building placement requires support and vegetation growth graph memberships.");
				break;
			}
		}
		break;
	}
	case VegetationCalibrationDomain::RepresentationFidelity: {
		const VegetationRepresentationFidelityProfile &fidelity =
			object_model.fidelityProfile();
		for (double threshold : {
		     fidelity.minimumSilhouetteMatchPercent(),
		     fidelity.minimumProjectedFoliageAreaMatchPercent(),
		     fidelity.minimumCrownGapFractionMatchPercent(),
		     fidelity.minimumBranchTopologyMatchPercent(),
		     fidelity.minimumMaterialAreaMatchPercent(),
		     fidelity.minimumMotionEnvelopeMatchPercent()}) {
			if (!valid_fidelity_threshold(threshold)) {
				unmet_requirements.push_back(
					"Representation fidelity requires valid independent metric thresholds.");
				break;
			}
		}
		if (biological_profile.evidence().sourceIdentifiers().empty()) {
			unmet_requirements.push_back(
				"Representation fidelity requires traceable grammar and camera evidence identifiers.");
		}
		break;
	}
	}
	return unmet_requirements;
}

}

VegetationCalibrationReadinessReport
VegetationCalibrationReadinessEvaluationService::evaluate(
	const BuildingVegetationObjectModel &object_model,
	const std::vector<VegetationCalibrationDomain> &requested_domains) const
{
	VegetationBiologicalProfileValidationReport validation_report =
		VegetationBiologicalProfileValidationService().validate(object_model);
	std::vector<VegetationCalibrationReadinessObservation> observations;
	std::set<VegetationCalibrationDomain> observed_domains;
	for (VegetationCalibrationDomain domain : requested_domains) {
		if (!observed_domains.insert(domain).second) continue;
		std::vector<std::string> unmet_requirements =
			unmet_requirements_for(object_model, domain);
		if (!validation_report.passed()) {
			unmet_requirements.push_back(
				"The biological profile must pass structural validation.");
		}
		observations.emplace_back(
			domain,
			unmet_requirements.empty(),
			std::move(unmet_requirements));
	}
	return VegetationCalibrationReadinessReport(
		std::move(validation_report), std::move(observations));
}
