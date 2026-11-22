#pragma once

#include "Mesh.h"
#include "geometry/model/FacialSkinMechanicalState.h"
#include "geometry/model/FacialSkinShapeSpecification.h"
#include "geometry/model/MeshSurfaceTag.h"

#include <vector>

class FacialElasticModelAssembler
{
public:
	FacialSkinMechanicalState assemble(
		const FacialSkinShapeSpecification &specification,
		const Mesh &neutral_mesh,
		const std::vector<MeshSurfaceTag> &surface_tags) const;
};
