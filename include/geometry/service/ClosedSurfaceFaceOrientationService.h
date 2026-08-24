#pragma once

#include "geometry/model/ClosedSurfaceFaceOrientationReport.h"

class Mesh;

class ClosedSurfaceFaceOrientationService
{
public:
	ClosedSurfaceFaceOrientationReport orientOutward(Mesh &mesh) const;
};
