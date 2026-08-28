#pragma once

#include "geometry/service/StlMeshLoadingService.h"
#include "vehicle/mcsmv2/model/McsMv22NativeKinematicEvidenceGeometry.h"
#include "vehicle/mcsmv2/service/McsMv22NativePartitionGeometryLoadingService.h"

#include <filesystem>
#include <string>

class McsMv22NativeKinematicEvidenceLoadingService
{
public:
	McsMv22NativeKinematicEvidenceGeometry load(
		const std::filesystem::path &source_release_root,
		const std::filesystem::path &native_partition_mesh_root,
		const std::string &variant_identifier,
		const glm::dmat4 &source_to_progen3d_matrix) const;

private:
	StlMeshLoadingService stl_loading_service_;
	McsMv22NativePartitionGeometryLoadingService partition_loading_service_;
};
