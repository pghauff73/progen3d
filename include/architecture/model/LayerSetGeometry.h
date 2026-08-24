#pragma once

#include "architecture/model/LayerGeometryPart.h"
#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <utility>
#include <vector>

class LayerSetGeometry
{
public:
	LayerSetGeometry(std::vector<LayerGeometryPart> layers,
	                 GeneratedPrimitiveMesh combined_preview_mesh)
		: layers_(std::move(layers)),
		  combined_preview_mesh_(std::move(combined_preview_mesh))
	{
	}

	const std::vector<LayerGeometryPart> &layers() const { return layers_; }
	const GeneratedPrimitiveMesh &combinedPreviewMesh() const
	{
		return combined_preview_mesh_;
	}

private:
	std::vector<LayerGeometryPart> layers_;
	GeneratedPrimitiveMesh combined_preview_mesh_{std::make_shared<Mesh>(), {}};
};
