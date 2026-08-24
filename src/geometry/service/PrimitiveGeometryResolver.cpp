#include "geometry/service/PrimitiveGeometryResolver.h"

#include "geometry/model/ShapeSpecification.h"
#include "geometry/model/MeshSurfaceTag.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "geometry/service/ProceduralMeshRepository.h"

#include <glm/gtc/constants.hpp>

#include <utility>

namespace {

bool is_cube_vertex_template_type(const std::string &primitive_type)
{
	return primitive_type == "Cube" ||
	       primitive_type == "CubeX" ||
	       primitive_type == "CubeY" ||
	       primitive_type == "CubeZ";
}

std::shared_ptr<const Mesh> share_static_mesh(const Mesh &mesh)
{
	return std::shared_ptr<const Mesh>(&mesh, [](const Mesh *) {});
}

}

PrimitiveGeometryResolver::PrimitiveGeometryResolver()
	: cube_vertex_template_(ResolvedPrimitiveGeometry::createCubeVertexTemplate()),
	  procedural_mesh_repository_(std::make_unique<ProceduralMeshRepository>())
{
}

PrimitiveGeometryResolver::~PrimitiveGeometryResolver() = default;

std::shared_ptr<const ResolvedPrimitiveGeometry>
PrimitiveGeometryResolver::resolvePrimitiveType(
	const std::string &primitive_type) const
{
	if (is_cube_vertex_template_type(primitive_type)) {
		return cube_vertex_template_;
	}
	return resolveTriangleMesh(primitive_type);
}

std::shared_ptr<const ResolvedPrimitiveGeometry>
PrimitiveGeometryResolver::resolvePrimitive(
	const std::string &primitive_type,
	const std::shared_ptr<const ShapeSpecification> &shape_specification,
	std::string *diagnostic) const
{
	if (shape_specification && !shape_specification->isDefaultFamilyShape()) {
		return procedural_mesh_repository_->resolve(*shape_specification, diagnostic);
	}
	return resolvePrimitiveType(primitive_type);
}

std::shared_ptr<const ResolvedPrimitiveGeometry>
PrimitiveGeometryResolver::resolveTriangleMesh(
	const std::string &primitive_type) const
{
	const auto existing = resolved_meshes_.find(primitive_type);
	if (existing != resolved_meshes_.end()) {
		return existing->second;
	}

	const Mesh &mesh = Mesh::getSharedInstance(primitive_type);
	if (mesh.faces.empty()) {
		return {};
	}

	std::vector<MeshSurfaceTag> tags(mesh.faces.size(),
	                                MeshSurfaceTag(MeshSurfaceRole::Outer));
	MeshTopologyAnalyzer analyzer;
	const MeshTopologyReport topology = analyzer.analyze(mesh, tags);
	PrimitiveVolumeEvidence volume = PrimitiveVolumeEvidence::createUndefined();
	PrimitiveCollisionPolicy collision_policy =
		PrimitiveCollisionPolicy::StaticTriangleMesh;
	if (primitive_type == "Cylinder") {
		volume = PrimitiveVolumeEvidence::createAnalytic(glm::pi<float>() * 0.25f);
		collision_policy = PrimitiveCollisionPolicy::ConvexMesh;
	}
	else if (primitive_type == "Sphere") {
		volume = PrimitiveVolumeEvidence::createAnalytic(glm::pi<float>() / 6.0f);
		collision_policy = PrimitiveCollisionPolicy::ConvexMesh;
	}
	auto resolved_geometry = ResolvedPrimitiveGeometry::createTriangleMesh(
		share_static_mesh(mesh),
		std::move(tags),
		topology.isWatertight(),
		volume,
		collision_policy,
		topology.topology_hash);
	resolved_meshes_.emplace(primitive_type, resolved_geometry);
	return resolved_geometry;
}
