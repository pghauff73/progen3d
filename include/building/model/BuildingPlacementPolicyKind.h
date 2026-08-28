#pragma once

enum class BuildingPlacementPolicyKind {
	AuthoredFixed,
	ConstraintSolved,
	HostedByObject,
	DerivedFromParent,
	AggregateFromChildren,
	NotApplicable
};
