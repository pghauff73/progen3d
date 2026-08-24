#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <cstddef>
#include <string>
#include <utility>

class SurfaceTilingGeometry
{
public:
	SurfaceTilingGeometry(std::size_t tile_count,
	                      std::string material_identifier,
	                      GeneratedPrimitiveMesh generated_mesh)
		: tile_count_(tile_count),
		  material_identifier_(std::move(material_identifier)),
		  generated_mesh_(std::move(generated_mesh))
	{
	}

	std::size_t tileCount() const { return tile_count_; }
	const std::string &materialIdentifier() const { return material_identifier_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }

private:
	std::size_t tile_count_ = 0;
	std::string material_identifier_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
};
