#pragma once

enum class BuildingExtentKind {
	Visual,
	Collision,
	Occupancy,
	Clearance,
	Access,
	Motion,
	Growth,
	Aggregate
};

enum class BuildingExtentApplicability {
	Applicable,
	Derived,
	NotApplicable
};

enum class BuildingExtentSourceKind {
	AuthoredBoundary,
	DirectGeometry,
	DescendantGeometry,
	BoundaryInflation,
	KinematicEnvelope,
	VegetationGrowthEnvelope,
	None
};
