#pragma once

#include "vegetation/model/PlantArchitecturalTopologyProfile.h"
#include "vegetation/model/PlantBiomechanicalProfile.h"
#include "vegetation/model/PlantCanopyOpticalProfile.h"
#include "vegetation/model/PlantEnvironmentalResponseProfile.h"
#include "vegetation/model/PlantFunctionalTraitProfile.h"
#include "vegetation/model/PlantHydraulicProfile.h"
#include "vegetation/model/PlantIdentityProfile.h"
#include "vegetation/model/PlantPhenologyProfile.h"
#include "vegetation/model/PlantRootArchitectureProfile.h"
#include "vegetation/model/PlantSemanticAnnotationProfile.h"
#include "vegetation/model/PlantSizeAllometryProfile.h"
#include "vegetation/model/VegetationEvidenceProfile.h"
#include "vegetation/model/VegetationSubstrateRequirementProfile.h"

#include <utility>

class VegetationBiologicalProfile
{
public:
	VegetationBiologicalProfile(
		PlantIdentityProfile identity,
		PlantArchitecturalTopologyProfile architectural_topology,
		PlantPhenologyProfile phenology,
		PlantRootArchitectureProfile root_architecture,
		PlantCanopyOpticalProfile canopy_optics,
		PlantBiomechanicalProfile biomechanics,
		PlantEnvironmentalResponseProfile environmental_response,
		VegetationEvidenceProfile evidence,
		PlantSemanticAnnotationProfile semantic_annotations =
			PlantSemanticAnnotationProfile::uncalibrated(),
		PlantFunctionalTraitProfile functional_traits =
			PlantFunctionalTraitProfile::uncalibrated(),
		PlantHydraulicProfile hydraulics = PlantHydraulicProfile::uncalibrated(),
		PlantSizeAllometryProfile size_allometry =
			PlantSizeAllometryProfile::uncalibrated(),
		VegetationSubstrateRequirementProfile substrate_requirements =
			VegetationSubstrateRequirementProfile::uncalibrated())
		: identity_(std::move(identity)),
		  architectural_topology_(std::move(architectural_topology)),
		  phenology_(std::move(phenology)),
		  root_architecture_(std::move(root_architecture)),
		  canopy_optics_(std::move(canopy_optics)),
		  biomechanics_(std::move(biomechanics)),
		  environmental_response_(std::move(environmental_response)),
		  evidence_(std::move(evidence)),
		  semantic_annotations_(std::move(semantic_annotations)),
		  functional_traits_(std::move(functional_traits)),
		  hydraulics_(std::move(hydraulics)),
		  size_allometry_(std::move(size_allometry)),
		  substrate_requirements_(std::move(substrate_requirements))
	{
	}

	static VegetationBiologicalProfile uncalibratedArchitecturalArchetype(
		PlantArchitecture architecture)
	{
		return VegetationBiologicalProfile(
			PlantIdentityProfile::architecturalArchetype(architecture),
			PlantArchitecturalTopologyProfile::multiscaleIndividualPlant(),
			PlantPhenologyProfile::uncalibratedMatureLeafOnProfile(),
			PlantRootArchitectureProfile::uncalibratedSafetyEnvelope(),
			PlantCanopyOpticalProfile::uncalibrated(),
			PlantBiomechanicalProfile::uncalibrated(),
			PlantEnvironmentalResponseProfile::uncalibratedRequiredDrivers(),
			VegetationEvidenceProfile::architecturalArchetype());
	}

	const PlantIdentityProfile &identity() const { return identity_; }
	const PlantArchitecturalTopologyProfile &architecturalTopology() const
	{
		return architectural_topology_;
	}
	const PlantPhenologyProfile &phenology() const { return phenology_; }
	const PlantRootArchitectureProfile &rootArchitecture() const
	{
		return root_architecture_;
	}
	const PlantCanopyOpticalProfile &canopyOptics() const
	{
		return canopy_optics_;
	}
	const PlantBiomechanicalProfile &biomechanics() const
	{
		return biomechanics_;
	}
	const PlantEnvironmentalResponseProfile &environmentalResponse() const
	{
		return environmental_response_;
	}
	const VegetationEvidenceProfile &evidence() const { return evidence_; }
	const PlantSemanticAnnotationProfile &semanticAnnotations() const
	{
		return semantic_annotations_;
	}
	const PlantFunctionalTraitProfile &functionalTraits() const
	{
		return functional_traits_;
	}
	const PlantHydraulicProfile &hydraulics() const { return hydraulics_; }
	const PlantSizeAllometryProfile &sizeAllometry() const
	{
		return size_allometry_;
	}
	const VegetationSubstrateRequirementProfile &substrateRequirements() const
	{
		return substrate_requirements_;
	}

private:
	PlantIdentityProfile identity_ =
		PlantIdentityProfile::architecturalArchetype(PlantArchitecture::Tree);
	PlantArchitecturalTopologyProfile architectural_topology_ =
		PlantArchitecturalTopologyProfile::multiscaleIndividualPlant();
	PlantPhenologyProfile phenology_ =
		PlantPhenologyProfile::uncalibratedMatureLeafOnProfile();
	PlantRootArchitectureProfile root_architecture_ =
		PlantRootArchitectureProfile::uncalibratedSafetyEnvelope();
	PlantCanopyOpticalProfile canopy_optics_ =
		PlantCanopyOpticalProfile::uncalibrated();
	PlantBiomechanicalProfile biomechanics_ =
		PlantBiomechanicalProfile::uncalibrated();
	PlantEnvironmentalResponseProfile environmental_response_ =
		PlantEnvironmentalResponseProfile::uncalibratedRequiredDrivers();
	VegetationEvidenceProfile evidence_ =
		VegetationEvidenceProfile::architecturalArchetype();
	PlantSemanticAnnotationProfile semantic_annotations_ =
		PlantSemanticAnnotationProfile::uncalibrated();
	PlantFunctionalTraitProfile functional_traits_ =
		PlantFunctionalTraitProfile::uncalibrated();
	PlantHydraulicProfile hydraulics_ = PlantHydraulicProfile::uncalibrated();
	PlantSizeAllometryProfile size_allometry_ =
		PlantSizeAllometryProfile::uncalibrated();
	VegetationSubstrateRequirementProfile substrate_requirements_ =
		VegetationSubstrateRequirementProfile::uncalibrated();
};
