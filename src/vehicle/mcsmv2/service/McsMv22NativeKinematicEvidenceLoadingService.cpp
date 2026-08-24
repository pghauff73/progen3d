#include "vehicle/mcsmv2/service/McsMv22NativeKinematicEvidenceLoadingService.h"

#include <stdexcept>
#include <utility>

McsMv22NativeKinematicEvidenceGeometry
McsMv22NativeKinematicEvidenceLoadingService::load(
	const std::filesystem::path &source_release_root,
	const std::filesystem::path &native_partition_mesh_root,
	const std::string &variant_identifier,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	const std::filesystem::path source_body_path =
		source_release_root / "models" /
		("modern_car_v2_" + variant_identifier + "_body.stl");
	if (!std::filesystem::is_regular_file(source_body_path)) {
		throw std::invalid_argument(
			"MCSMv2.2 signed final-body STL does not exist: " +
			source_body_path.string());
	}
	GeneratedPrimitiveMesh source_final_body =
		stl_loading_service_.loadWeldedMesh(source_body_path, 1.0e-6);
	McsMv22NativePartitionGeometry partition_geometry =
		partition_loading_service_.loadProgen3dGeometry(
			native_partition_mesh_root, variant_identifier,
			source_to_progen3d_matrix);
	return McsMv22NativeKinematicEvidenceGeometry(
		variant_identifier, std::move(source_final_body),
		std::move(partition_geometry));
}
