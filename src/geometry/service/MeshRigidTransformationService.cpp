#include "geometry/service/MeshRigidTransformationService.h"

#include <memory>

GeneratedPrimitiveMesh MeshRigidTransformationService::transform(
	const GeneratedPrimitiveMesh &source_mesh,
	const glm::dmat4 &rigid_transform) const
{
	auto transformed_mesh = std::make_shared<Mesh>(*source_mesh.mesh());
	transformed_mesh->apply(glm::mat4(rigid_transform));
	transformed_mesh->calc_normals();
	return GeneratedPrimitiveMesh(
		std::move(transformed_mesh), source_mesh.faceSurfaceTags());
}
