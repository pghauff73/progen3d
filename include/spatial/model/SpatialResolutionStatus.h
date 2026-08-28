#pragma once

enum class SpatialResolutionStatus {
	Succeeded,
	Failed,
	Rejected
};

enum class SpatialResolutionAlgorithm {
	AxisAlignedSweep,
	OpeningBoundaryFit,
	AuthoredPlacement,
	ValidationOnly
};
