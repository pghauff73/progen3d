#pragma once

#include "chair/model/ChairThreeViewFitReport.h"
#include "geometry/model/ThreeViewProjection.h"

#include <string>

class ChairThreeViewFittingService
{
public:
	ChairThreeViewFitReport compare(
		const std::string &chair_identifier,
		const ThreeViewProjection &candidate,
		const ThreeViewProjection &reference,
		int matched_iteration,
		std::string *diagnostic = nullptr) const;
};
