#pragma once

#include "Mesh.h"
#include "geometry/model/MeshSurfaceTag.h"

#include <memory>
#include <utility>
#include <vector>

class GeneratedPrimitiveMesh
{
public:
	GeneratedPrimitiveMesh(std::shared_ptr<Mesh> mesh,
	                       std::vector<MeshSurfaceTag> face_surface_tags)
		: mesh_(std::move(mesh)),
		  face_surface_tags_(std::move(face_surface_tags))
	{
	}

	const std::shared_ptr<Mesh> &mesh() const
	{
		return mesh_;
	}

	const std::vector<MeshSurfaceTag> &faceSurfaceTags() const
	{
		return face_surface_tags_;
	}

private:
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};
