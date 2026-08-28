#pragma once

#include "spatial/model/SpatialConstraintId.h"

enum class SpatialConstraintKind {
	CollisionPosition,
	Clearance,
	Containment
};

class SpatialConstraint {
public:
	virtual ~SpatialConstraint() = default;
	virtual SpatialConstraintKind kind() const = 0;
	virtual const SpatialConstraintId &constraintId() const = 0;
	virtual int priority() const = 0;
};
