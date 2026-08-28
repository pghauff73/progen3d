#pragma once

#include "building/model/BuildingObjectSpatialProfile.h"
#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationBiologicalProfile.h"
#include "vegetation/model/VegetationRepresentationFidelityProfile.h"
#include "vegetation/model/VegetationTriangleDistributionPolicy.h"

#include <string>
#include <utility>

class BuildingVegetationObjectModel
{
public:
	BuildingVegetationObjectModel(
		std::string model_identifier,
		std::string display_name,
		PlantArchitecture plant_architecture,
		BuildingObjectSpatialProfile spatial_profile,
		VegetationRepresentationFidelityProfile fidelity_profile,
		VegetationTriangleDistributionPolicy triangle_distribution_policy)
		: BuildingVegetationObjectModel(
			  std::move(model_identifier),
			  std::move(display_name),
			  plant_architecture,
			  std::move(spatial_profile),
			  std::move(fidelity_profile),
			  std::move(triangle_distribution_policy),
			  VegetationBiologicalProfile::uncalibratedArchitecturalArchetype(
				  plant_architecture))
	{
	}

	BuildingVegetationObjectModel(
		std::string model_identifier,
		std::string display_name,
		PlantArchitecture plant_architecture,
		BuildingObjectSpatialProfile spatial_profile,
		VegetationRepresentationFidelityProfile fidelity_profile,
		VegetationTriangleDistributionPolicy triangle_distribution_policy,
		VegetationBiologicalProfile biological_profile)
		: model_identifier_(std::move(model_identifier)),
		  display_name_(std::move(display_name)),
		  plant_architecture_(plant_architecture),
		  spatial_profile_(std::move(spatial_profile)),
		  fidelity_profile_(std::move(fidelity_profile)),
		  triangle_distribution_policy_(
			  std::move(triangle_distribution_policy)),
		  biological_profile_(std::move(biological_profile))
	{
	}

	const std::string &modelIdentifier() const { return model_identifier_; }
	const std::string &displayName() const { return display_name_; }
	PlantArchitecture plantArchitecture() const { return plant_architecture_; }
	const BuildingObjectSpatialProfile &spatialProfile() const
	{
		return spatial_profile_;
	}
	const VegetationRepresentationFidelityProfile &fidelityProfile() const
	{
		return fidelity_profile_;
	}
	const VegetationTriangleDistributionPolicy &triangleDistributionPolicy() const
	{
		return triangle_distribution_policy_;
	}
	const VegetationBiologicalProfile &biologicalProfile() const
	{
		return biological_profile_;
	}

private:
	std::string model_identifier_;
	std::string display_name_;
	PlantArchitecture plant_architecture_ = PlantArchitecture::Tree;
	BuildingObjectSpatialProfile spatial_profile_{
		SpatialObjectId("UnspecifiedVegetationObject"),
		BuildingSpatialManifestationKind::SemanticOnly,
		BuildingPlacementPolicyKind::NotApplicable,
		BuildingCollisionBehaviorKind::NonParticipating,
		BuildingObjectExtentSet(),
		BuildingOrientationProfile::notApplicable(),
		{}, {}, 0u, 0u};
	VegetationRepresentationFidelityProfile fidelity_profile_{
		95.0, 95.0, 95.0, 95.0, 95.0, 95.0};
	VegetationTriangleDistributionPolicy triangle_distribution_policy_{0u};
	VegetationBiologicalProfile biological_profile_ =
		VegetationBiologicalProfile::uncalibratedArchitecturalArchetype(
			PlantArchitecture::Tree);
};
