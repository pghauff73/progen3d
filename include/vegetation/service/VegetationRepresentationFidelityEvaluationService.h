#pragma once

#include "vegetation/model/VegetationRepresentationFidelityProfile.h"
#include "vegetation/model/VegetationRepresentationFidelityReport.h"
#include "vegetation/model/VegetationViewFidelityObservation.h"

#include <vector>

class VegetationRepresentationFidelityEvaluationService
{
public:
	VegetationRepresentationFidelityReport evaluate(
		const VegetationRepresentationFidelityProfile &profile,
		const std::vector<VegetationViewFidelityObservation> &observations) const;
};
