#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <map>
#include <string>
#include <utility>

class McsMv22NativePartitionGeometry
{
public:
	McsMv22NativePartitionGeometry(
		std::string variant_identifier,
		GeneratedPrimitiveMesh fixed_body_mesh,
		std::map<std::string, GeneratedPrimitiveMesh> closure_meshes,
		std::map<std::string, GeneratedPrimitiveMesh> glass_meshes)
		: variant_identifier_(std::move(variant_identifier)),
		  fixed_body_mesh_(std::move(fixed_body_mesh)),
		  closure_meshes_(std::move(closure_meshes)),
		  glass_meshes_(std::move(glass_meshes))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const GeneratedPrimitiveMesh &fixedBodyMesh() const { return fixed_body_mesh_; }
	const std::map<std::string, GeneratedPrimitiveMesh> &closureMeshes() const
	{
		return closure_meshes_;
	}
	const std::map<std::string, GeneratedPrimitiveMesh> &glassMeshes() const
	{
		return glass_meshes_;
	}

private:
	std::string variant_identifier_;
	GeneratedPrimitiveMesh fixed_body_mesh_{std::make_shared<Mesh>(), {}};
	std::map<std::string, GeneratedPrimitiveMesh> closure_meshes_;
	std::map<std::string, GeneratedPrimitiveMesh> glass_meshes_;
};
