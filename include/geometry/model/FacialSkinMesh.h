#pragma once

#include "Mesh.h"
#include "geometry/model/FacialSkinLandmarks.h"
#include "geometry/model/FacialSkinMechanicalState.h"

#include <memory>

// A facial triangle mesh carries semantic landmarks and, for elastic mode,
// the material/attachment model and solve evidence that produced its vertices.
class FacialSkinMesh : public Mesh
{
public:
	FacialSkinLandmarks landmarks;
	std::shared_ptr<const FacialSkinMechanicalState> mechanical_state;
};
