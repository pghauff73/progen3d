#pragma once

#include "architecture/model/LayerDefinition.h"
#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/ShapeSpecification.h"

#include <memory>
#include <utility>

class LayerGeometryPart
{
public:
	LayerGeometryPart(LayerDefinition definition,
	                  float axial_offset,
	                  std::shared_ptr<const ShapeSpecification> shape,
	                  GeneratedPrimitiveMesh generated_mesh)
		: definition_(std::move(definition)),
		  axial_offset_(axial_offset),
		  shape_(std::move(shape)),
		  generated_mesh_(std::move(generated_mesh))
	{
	}

	const LayerDefinition &definition() const { return definition_; }
	float axialOffset() const { return axial_offset_; }
	const std::shared_ptr<const ShapeSpecification> &shape() const { return shape_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }

private:
	LayerDefinition definition_{"", 0.0f, ""};
	float axial_offset_ = 0.0f;
	std::shared_ptr<const ShapeSpecification> shape_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
};
