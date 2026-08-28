#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/model/VehicleTyrePoseSweep.h"

#include <glm/glm.hpp>

class VehicleTyrePoseTransformationService
{
public:
	GeneratedPrimitiveMesh transformSourceTyre(
		const GeneratedPrimitiveMesh &source_tyre_mesh,
		const VehicleTyrePoseSample &pose_sample,
		const glm::dmat4 &source_to_progen3d_matrix) const;
};
