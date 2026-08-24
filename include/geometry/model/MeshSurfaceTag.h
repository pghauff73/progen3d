#pragma once

#include <cstddef>

enum class MeshSurfaceRole
{
	Outer,
	Inner,
	AxialStart,
	AxialEnd,
	AngularStart,
	AngularEnd,
	PolarStart,
	PolarEnd,
	Clip,
	Rim,
	AxialSide,
	AxialStep,
	AxialBottomCap,
	AxialTopCap,
	ProfileOuterSide,
	ProfileInnerSide,
	ProfileFrontCap,
	ProfileBackCap,
	HostedOpeningCutWall,
	EmbossedBeadOuter,
	EdgeFlangeOuter,
	BotanicalBranchSide,
	BotanicalBranchBase,
	BotanicalBranchTip,
	BotanicalBladeUpper,
	BotanicalBladeLower,
	BotanicalBladeEdge,
	BotanicalPetalUpper,
	BotanicalPetalLower,
	BotanicalPetalEdge,
	BotanicalJunction,
	BotanicalFruitOuter
};

class MeshSurfaceTag
{
public:
	MeshSurfaceTag(MeshSurfaceRole role = MeshSurfaceRole::Outer,
	               std::size_t boundary_index = 0,
	               std::size_t object_part_index = 0)
		: role_(role),
		  boundary_index_(boundary_index),
		  object_part_index_(object_part_index)
	{
	}

	MeshSurfaceRole role() const
	{
		return role_;
	}

	std::size_t boundaryIndex() const
	{
		return boundary_index_;
	}

	std::size_t objectPartIndex() const
	{
		return object_part_index_;
	}

private:
	MeshSurfaceRole role_ = MeshSurfaceRole::Outer;
	std::size_t boundary_index_ = 0;
	std::size_t object_part_index_ = 0;
};
