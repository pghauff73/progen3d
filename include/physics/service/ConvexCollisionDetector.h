#pragma once

#include "physics/model/CollisionContact.h"
#include "physics/model/ConvexCollisionShape.h"

class ConvexCollisionDetector
{
public:
	CollisionContact detect(const ConvexCollisionShape &first,
	                        const ConvexCollisionShape &second) const;
};
