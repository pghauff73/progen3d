#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <string>
#include <utility>

class PlantOrganGeometrySource
{
public:
	PlantOrganGeometrySource(
		std::shared_ptr<const ShapeSpecification> shape,
		GeneratedPrimitiveMesh generated_mesh,
		std::string semantic_role,
		std::string material_identifier)
		: shape_(std::move(shape)),
		  generated_mesh_(std::move(generated_mesh)),
		  semantic_role_(std::move(semantic_role)),
		  material_identifier_(std::move(material_identifier))
	{
	}

	const std::shared_ptr<const ShapeSpecification> &shape() const
	{
		return shape_;
	}
	const GeneratedPrimitiveMesh &generatedMesh() const
	{
		return generated_mesh_;
	}
	const std::string &semanticRole() const { return semantic_role_; }
	const std::string &materialIdentifier() const
	{
		return material_identifier_;
	}

private:
	std::shared_ptr<const ShapeSpecification> shape_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	std::string semantic_role_;
	std::string material_identifier_;
};
