#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <filesystem>

class StlMeshLoadingService
{
public:
	GeneratedPrimitiveMesh loadWeldedMesh(
		const std::filesystem::path &stl_path,
		double weld_quantization_metres = 1.0e-6) const;
};
