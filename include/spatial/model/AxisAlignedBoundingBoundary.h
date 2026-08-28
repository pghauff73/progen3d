#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/BoundaryRepresentation.h"

class AxisAlignedBoundingBoundary : public BoundaryRepresentation {
public:
	explicit AxisAlignedBoundingBoundary(AxisAlignedBounds bounds) : bounds_(bounds) {}

	BoundaryRepresentationKind kind() const override
	{
		return BoundaryRepresentationKind::AxisAlignedBounding;
	}

	const AxisAlignedBounds &bounds() const { return bounds_; }

private:
	AxisAlignedBounds bounds_;
};
