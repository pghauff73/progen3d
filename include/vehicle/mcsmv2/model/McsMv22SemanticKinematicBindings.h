#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <map>
#include <string>
#include <utility>

class McsMv22SemanticKinematicBindings
{
public:
	McsMv22SemanticKinematicBindings(
		GeneratedPrimitiveMesh final_body_mesh,
		GeneratedPrimitiveMesh fixed_body_mesh,
		std::map<std::string, GeneratedPrimitiveMesh> closure_meshes,
		std::map<std::string, GeneratedPrimitiveMesh> glass_meshes)
		: final_body_mesh_(std::move(final_body_mesh)),
		  fixed_body_mesh_(std::move(fixed_body_mesh)),
		  closure_meshes_(std::move(closure_meshes)),
		  glass_meshes_(std::move(glass_meshes))
	{
	}

	const GeneratedPrimitiveMesh &finalBodyMesh() const { return final_body_mesh_; }
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
	GeneratedPrimitiveMesh final_body_mesh_{std::make_shared<Mesh>(), {}};
	GeneratedPrimitiveMesh fixed_body_mesh_{std::make_shared<Mesh>(), {}};
	std::map<std::string, GeneratedPrimitiveMesh> closure_meshes_;
	std::map<std::string, GeneratedPrimitiveMesh> glass_meshes_;
};
