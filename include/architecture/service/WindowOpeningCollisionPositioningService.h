#pragma once

#include "architecture/model/OpeningBoundaryPlacementResult.h"
#include "architecture/model/WindowFrameSpecification.h"
#include "architecture/model/WindowOpeningSpecification.h"

class WindowOpeningCollisionPositioningService
{
public:
	OpeningBoundaryPlacementResult position(
		const WindowFrameSpecification &frame,
		const WindowOpeningSpecification &opening,
		float tolerance = 0.0005f) const;
};
