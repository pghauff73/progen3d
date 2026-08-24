#pragma once

#include "architecture/model/ArchitecturalAssemblyPart.h"
#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "spatial/model/SpatialInterface.h"

#include <utility>
#include <vector>

class ArchitecturalAssemblyGeometry
{
public:
	ArchitecturalAssemblyGeometry(
		std::vector<ArchitecturalAssemblyPart> parts,
		GeneratedPrimitiveMesh combined_preview_mesh,
		std::vector<SpatialInterface> interfaces,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: parts_(std::move(parts)),
		  combined_preview_mesh_(std::move(combined_preview_mesh)),
		  interfaces_(std::move(interfaces)),
		  detail_level_(detail_level)
	{
	}

	const std::vector<ArchitecturalAssemblyPart> &parts() const { return parts_; }
	const GeneratedPrimitiveMesh &combinedPreviewMesh() const
	{
		return combined_preview_mesh_;
	}
	const std::vector<SpatialInterface> &interfaces() const { return interfaces_; }
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::vector<ArchitecturalAssemblyPart> parts_;
	GeneratedPrimitiveMesh combined_preview_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<SpatialInterface> interfaces_;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::FastenersAndSeals;
};
