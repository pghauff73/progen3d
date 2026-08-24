#pragma once

#include "geometry/model/ResolvedPrimitiveGeometry.h"

#include <memory>
#include <string>
#include <unordered_map>

class ShapeSpecification;
class ProceduralMeshRepository;

class PrimitiveGeometryResolver
{
public:
	PrimitiveGeometryResolver();
	~PrimitiveGeometryResolver();

	std::shared_ptr<const ResolvedPrimitiveGeometry> resolvePrimitiveType(
		const std::string &primitive_type) const;
	std::shared_ptr<const ResolvedPrimitiveGeometry> resolvePrimitive(
		const std::string &primitive_type,
		const std::shared_ptr<const ShapeSpecification> &shape_specification,
		std::string *diagnostic = nullptr) const;

private:
	std::shared_ptr<const ResolvedPrimitiveGeometry> resolveTriangleMesh(
		const std::string &primitive_type) const;

	std::shared_ptr<const ResolvedPrimitiveGeometry> cube_vertex_template_;
	std::unique_ptr<ProceduralMeshRepository> procedural_mesh_repository_;
	mutable std::unordered_map<std::string,
	                           std::shared_ptr<const ResolvedPrimitiveGeometry>>
		resolved_meshes_;
};
