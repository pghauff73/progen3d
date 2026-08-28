#pragma once

#include "vegetation/model/VegetationPointCloudFormatCapability.h"

#include <optional>
#include <string>
#include <vector>

class VegetationPointCloudFormatCapabilityCatalog
{
public:
	std::vector<VegetationPointCloudFormatCapability> capabilities() const;
	std::optional<VegetationPointCloudFormatCapability> findCapability(
		const std::string &media_type,
		const std::string &source_schema_version,
		VegetationPointCloudEncoding encoding) const;
};
