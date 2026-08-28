#pragma once

#include "geometry/service/MeshRigidTransformationService.h"
#include "geometry/service/StlMeshLoadingService.h"
#include "vehicle/mcsmv2/model/McsMv22NativePartitionGeometry.h"

#include <filesystem>
#include <string>

class McsMv22NativePartitionGeometryLoadingService
{
public:
	McsMv22NativePartitionGeometry loadProgen3dGeometry(
		const std::filesystem::path &partition_mesh_root,
		const std::string &variant_identifier,
		const glm::dmat4 &source_to_progen3d_matrix) const;

private:
	GeneratedPrimitiveMesh loadTransformedMesh(
		const std::filesystem::path &variant_directory,
		const std::string &geometry_name,
		const glm::dmat4 &source_to_progen3d_matrix) const;

	StlMeshLoadingService stl_loading_service_;
	MeshRigidTransformationService transformation_service_;
};
