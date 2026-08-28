#pragma once

enum class GeometryBuildStatus
{
	Success,
	InvalidSpecification,
	InvalidProfile,
	SelfIntersection,
	InvalidHole,
	DegenerateFace,
	TriangulationFailure,
	NonFiniteGeometry,
	TriangleLimitExceeded,
	UnsupportedTopology
};
