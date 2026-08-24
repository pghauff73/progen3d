#pragma once

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class CompoundLeafPlacement
{
public:
	CompoundLeafPlacement(
		std::vector<glm::mat4> rachis_transforms,
		std::vector<glm::mat4> leaflet_transforms)
		: rachis_transforms_(std::move(rachis_transforms)),
		  leaflet_transforms_(std::move(leaflet_transforms))
	{
	}

	const std::vector<glm::mat4> &rachisTransforms() const
	{
		return rachis_transforms_;
	}

	const std::vector<glm::mat4> &leafletTransforms() const
	{
		return leaflet_transforms_;
	}

private:
	std::vector<glm::mat4> rachis_transforms_;
	std::vector<glm::mat4> leaflet_transforms_;
};

