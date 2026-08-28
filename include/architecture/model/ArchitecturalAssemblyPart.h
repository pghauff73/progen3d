#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryDetailRange.h"
#include "geometry/model/ShapeSpecification.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <utility>

class ArchitecturalAssemblyPart
{
public:
	ArchitecturalAssemblyPart(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		std::shared_ptr<const ShapeSpecification> shape,
		GeneratedPrimitiveMesh generated_mesh,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range = GeometryDetailRange())
		: part_identifier_(std::move(part_identifier)),
		  semantic_role_(std::move(semantic_role)),
		  material_identifier_(std::move(material_identifier)),
		  shape_(std::move(shape)),
		  generated_mesh_(std::move(generated_mesh)),
		  local_transform_(local_transform),
		  detail_range_(detail_range)
	{
	}

	const std::string &partIdentifier() const { return part_identifier_; }
	const std::string &semanticRole() const { return semantic_role_; }
	const std::string &materialIdentifier() const { return material_identifier_; }
	const std::shared_ptr<const ShapeSpecification> &shape() const { return shape_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }
	const glm::mat4 &localTransform() const { return local_transform_; }
	const GeometryDetailRange &detailRange() const { return detail_range_; }
	bool isVisibleAt(GeometryDetailLevel detail_level) const
	{
		return detail_range_.includes(detail_level);
	}

private:
	std::string part_identifier_;
	std::string semantic_role_;
	std::string material_identifier_;
	std::shared_ptr<const ShapeSpecification> shape_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	glm::mat4 local_transform_{1.0f};
	GeometryDetailRange detail_range_;
};
