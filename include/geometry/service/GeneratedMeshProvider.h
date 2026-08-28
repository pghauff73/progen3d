#pragma once

#include "geometry/model/GeneratedMeshResolutionResult.h"
#include "geometry/model/GeometryDetailLevel.h"

#include <string>

class GeneratedMeshProvider
{
public:
	virtual ~GeneratedMeshProvider() = default;

	virtual bool recognizes(const std::string &mesh_key) const = 0;
	virtual GeneratedMeshResolutionResult resolve(
		const std::string &mesh_key,
		GeometryDetailLevel detail_level) const = 0;
};
