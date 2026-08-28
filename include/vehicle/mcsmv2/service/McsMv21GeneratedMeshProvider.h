#pragma once

#include "geometry/service/GeneratedMeshProvider.h"

class McsMv21GeneratedMeshProvider final : public GeneratedMeshProvider
{
public:
	bool recognizes(const std::string &mesh_key) const override;
	GeneratedMeshResolutionResult resolve(
		const std::string &mesh_key,
		GeometryDetailLevel detail_level) const override;
};
