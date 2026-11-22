#pragma once

#include "geometry/model/FacialBendingAssemblyDiagnostics.h"
#include "geometry/model/FacialElasticSystem.h"

// Reconstructs the full curvature tensor from connected material neighborhoods.
// It changes neither the neutral surface nor the declared physical rigidity.
class FacialBendingStencilAssembler
{
public:
	FacialBendingAssemblyDiagnostics assemble(FacialElasticSystem &system,
	                                        double stiffness_scale) const;
};
