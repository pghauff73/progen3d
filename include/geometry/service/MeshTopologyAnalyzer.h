#pragma once

#include "Mesh.h"
#include "geometry/model/MeshSurfaceTag.h"
#include "geometry/model/MeshTopologyReport.h"

#include <vector>

class MeshTopologyAnalyzer
{
public:
	MeshTopologyReport analyze(
		const Mesh &mesh,
		const std::vector<MeshSurfaceTag> &face_surface_tags) const;
};
