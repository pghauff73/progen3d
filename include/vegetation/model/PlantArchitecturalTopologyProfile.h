#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationObjectCompositionKind
{
	IndividualPlant,
	ClonalColony,
	PlantCluster,
	MixedPlanting,
	HostedPlantSystem
};

enum class PlantArchitecturalScale
{
	WholePlant,
	Axis,
	GrowthUnit,
	Metamer,
	Organ,
	GeometryRegion,
	RepresentationCluster
};

enum class PlantTopologyRelationshipKind
{
	Decomposition,
	Succession,
	Branching,
	Attachment
};

class PlantArchitecturalTopologyProfile
{
public:
	PlantArchitecturalTopologyProfile(
		VegetationObjectCompositionKind composition,
		std::vector<PlantArchitecturalScale> scales,
		std::vector<PlantTopologyRelationshipKind> relationship_kinds,
		bool includes_root_system,
		bool includes_shoot_system,
		std::vector<std::string> topology_evidence_identifiers = {},
		std::optional<std::size_t> maximum_branch_order = std::nullopt)
		: composition_(composition),
		  scales_(std::move(scales)),
		  relationship_kinds_(std::move(relationship_kinds)),
		  includes_root_system_(includes_root_system),
		  includes_shoot_system_(includes_shoot_system),
		  topology_evidence_identifiers_(
			  std::move(topology_evidence_identifiers)),
		  maximum_branch_order_(maximum_branch_order)
	{
	}

	static PlantArchitecturalTopologyProfile multiscaleIndividualPlant()
	{
		return PlantArchitecturalTopologyProfile(
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
			true);
	}

	VegetationObjectCompositionKind composition() const { return composition_; }
	const std::vector<PlantArchitecturalScale> &scales() const { return scales_; }
	const std::vector<PlantTopologyRelationshipKind> &relationshipKinds() const
	{
		return relationship_kinds_;
	}
	bool includesRootSystem() const { return includes_root_system_; }
	bool includesShootSystem() const { return includes_shoot_system_; }
	const std::vector<std::string> &topologyEvidenceIdentifiers() const
	{
		return topology_evidence_identifiers_;
	}
	std::optional<std::size_t> maximumBranchOrder() const
	{
		return maximum_branch_order_;
	}
	bool hasCalibratedShootTopology() const
	{
		return includes_shoot_system_ &&
		       maximum_branch_order_.has_value() &&
		       maximum_branch_order_.value() > 0u &&
		       !topology_evidence_identifiers_.empty();
	}

private:
	VegetationObjectCompositionKind composition_ =
		VegetationObjectCompositionKind::IndividualPlant;
	std::vector<PlantArchitecturalScale> scales_;
	std::vector<PlantTopologyRelationshipKind> relationship_kinds_;
	bool includes_root_system_ = true;
	bool includes_shoot_system_ = true;
	std::vector<std::string> topology_evidence_identifiers_;
	std::optional<std::size_t> maximum_branch_order_;
};
