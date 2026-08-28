#pragma once

enum class BuildingCollisionBehaviorKind {
	HardBoundary,
	SoftBoundary,
	OccupancyRegion,
	VoidBoundary,
	AggregateBoundary,
	NonBlockingAggregate,
	NonParticipating
};
