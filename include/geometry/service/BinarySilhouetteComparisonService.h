#pragma once

#include "geometry/model/BinarySilhouette.h"
#include "geometry/model/SilhouetteComparisonResult.h"

#include <string>

class BinarySilhouetteComparisonService
{
public:
	SilhouetteComparisonResult compare(
		const BinarySilhouette &candidate,
		const BinarySilhouette &reference,
		std::string *diagnostic = nullptr) const;
};
