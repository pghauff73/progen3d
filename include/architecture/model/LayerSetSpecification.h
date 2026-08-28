#pragma once

#include "architecture/model/LayerDefinition.h"
#include "geometry/model/Profile2D.h"

#include <utility>
#include <vector>

class LayerSetSpecification
{
public:
	LayerSetSpecification(Profile2D reference_profile,
	                      std::vector<LayerDefinition> layers)
		: reference_profile_(std::move(reference_profile)),
		  layers_(std::move(layers))
	{
	}

	const Profile2D &referenceProfile() const { return reference_profile_; }
	const std::vector<LayerDefinition> &layers() const { return layers_; }

	float totalThickness() const
	{
		float thickness = 0.0f;
		for (const LayerDefinition &layer : layers_) thickness += layer.thickness();
		return thickness;
	}

private:
	Profile2D reference_profile_{{{}, ProfileWindingCorrection::None}, {}};
	std::vector<LayerDefinition> layers_;
};
