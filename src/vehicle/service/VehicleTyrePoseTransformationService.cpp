#include "vehicle/service/VehicleTyrePoseTransformationService.h"

#include <memory>
#include <vector>

GeneratedPrimitiveMesh VehicleTyrePoseTransformationService::transformSourceTyre(
	const GeneratedPrimitiveMesh &source_tyre_mesh,
	const VehicleTyrePoseSample &pose_sample,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	auto transformed_mesh = std::make_shared<Mesh>(*source_tyre_mesh.mesh());
	const glm::dmat4 direct_transform =
		source_to_progen3d_matrix * pose_sample.wheelPose().sourceTransform();
	transformed_mesh->apply(glm::mat4(direct_transform));
	transformed_mesh->calc_normals();
	return GeneratedPrimitiveMesh(
		std::move(transformed_mesh), source_tyre_mesh.faceSurfaceTags());
}
