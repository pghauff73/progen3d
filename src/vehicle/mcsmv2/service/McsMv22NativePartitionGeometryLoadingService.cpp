#include "vehicle/mcsmv2/service/McsMv22NativePartitionGeometryLoadingService.h"

#include <array>
#include <map>
#include <stdexcept>
#include <utility>

McsMv22NativePartitionGeometry
McsMv22NativePartitionGeometryLoadingService::loadProgen3dGeometry(
	const std::filesystem::path &partition_mesh_root,
	const std::string &variant_identifier,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	static constexpr std::array<const char *, 6> closure_names = {
		"front_door_left", "front_door_right", "rear_door_left",
		"rear_door_right", "hood", "rear_hatch"};
	static constexpr std::array<const char *, 4> glass_names = {
		"front_side_glass_left", "front_side_glass_right",
		"rear_side_glass_left", "rear_side_glass_right"};
	const std::filesystem::path variant_directory =
		partition_mesh_root / variant_identifier;
	if (!std::filesystem::is_directory(variant_directory)) {
		throw std::invalid_argument(
			"MCSMv2.2 native partition variant directory does not exist: " +
			variant_directory.string());
	}
	GeneratedPrimitiveMesh fixed_body = loadTransformedMesh(
		variant_directory, "panel_fixed_body", source_to_progen3d_matrix);
	std::map<std::string, GeneratedPrimitiveMesh> closures;
	for (const char *closure_name : closure_names) {
		closures.emplace(
			closure_name,
			loadTransformedMesh(
				variant_directory, "panel_" + std::string(closure_name),
				source_to_progen3d_matrix));
	}
	std::map<std::string, GeneratedPrimitiveMesh> glass;
	for (const char *glass_name : glass_names) {
		glass.emplace(
			glass_name,
			loadTransformedMesh(
				variant_directory, "aperture_" + std::string(glass_name),
				source_to_progen3d_matrix));
	}
	return McsMv22NativePartitionGeometry(
		variant_identifier, std::move(fixed_body), std::move(closures),
		std::move(glass));
}

GeneratedPrimitiveMesh
McsMv22NativePartitionGeometryLoadingService::loadTransformedMesh(
	const std::filesystem::path &variant_directory,
	const std::string &geometry_name,
	const glm::dmat4 &source_to_progen3d_matrix) const
{
	const std::filesystem::path mesh_path =
		variant_directory / (geometry_name + ".stl");
	const GeneratedPrimitiveMesh source_mesh =
		stl_loading_service_.loadWeldedMesh(mesh_path, 1.0e-6);
	return transformation_service_.transform(
		source_mesh, source_to_progen3d_matrix);
}
