#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <utility>

class ArchitecturalPartGeometry
{
public:
	ArchitecturalPartGeometry(
		std::shared_ptr<const ShapeSpecification> shape,
		GeneratedPrimitiveMesh generated_mesh)
		: shape_(std::move(shape)),
		  generated_mesh_(std::move(generated_mesh))
	{
	}

	bool isValid() const
	{
		return shape_ && generated_mesh_.mesh() &&
		       !generated_mesh_.mesh()->faces.empty();
	}

	const std::shared_ptr<const ShapeSpecification> &shape() const
	{
		return shape_;
	}

	const GeneratedPrimitiveMesh &generatedMesh() const
	{
		return generated_mesh_;
	}

private:
	std::shared_ptr<const ShapeSpecification> shape_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
};
