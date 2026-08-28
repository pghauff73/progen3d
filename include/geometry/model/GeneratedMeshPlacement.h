#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

class GeneratedMeshPlacement
{
public:
	GeneratedMeshPlacement(
		std::string purpose,
		GeneratedPrimitiveMesh generated_mesh,
		glm::mat4 local_transform)
		: purpose_(std::move(purpose)),
		  generated_mesh_(std::move(generated_mesh)),
		  local_transform_(local_transform)
	{
	}

	const std::string &purpose() const { return purpose_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string purpose_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	glm::mat4 local_transform_{1.0f};
};
