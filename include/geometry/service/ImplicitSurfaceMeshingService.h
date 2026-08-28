#pragma once

#include "geometry/model/ImplicitScalarField.h"
#include "geometry/model/ImplicitSurfaceGenerationRequest.h"
#include "geometry/model/ImplicitSurfaceGenerationResult.h"

class ImplicitSurfaceMeshingService
{
public:
	ImplicitSurfaceGenerationResult generate(
		const ImplicitScalarField &field,
		const ImplicitSurfaceGenerationRequest &request) const;
};
