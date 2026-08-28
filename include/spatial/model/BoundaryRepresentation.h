#pragma once

enum class BoundaryRepresentationKind {
	AxisAlignedBounding,
	OrientedBounding,
	ConvexHull,
	TriangleMesh,
	AnalyticSurface,
	Compound
};

class BoundaryRepresentation {
public:
	virtual ~BoundaryRepresentation() = default;
	virtual BoundaryRepresentationKind kind() const = 0;
};
