#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vegetation/model/VegetationGeometryPart.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class VegetationAssemblyGeometry
{
public:
	VegetationAssemblyGeometry(
		std::vector<VegetationGeometryPart> parts,
		GeneratedPrimitiveMesh combined_preview_mesh,
		GeometryDetailLevel detail_level)
		: parts_(std::move(parts)),
		  combined_preview_mesh_(std::move(combined_preview_mesh)),
		  detail_level_(detail_level)
	{
	}

	const std::vector<VegetationGeometryPart> &parts() const { return parts_; }
	const GeneratedPrimitiveMesh &combinedPreviewMesh() const
	{
		return combined_preview_mesh_;
	}
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::vector<VegetationGeometryPart> parts_;
	GeneratedPrimitiveMesh combined_preview_mesh_{std::make_shared<Mesh>(), {}};
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::Component;
};

class VegetationGeometryBuildResult
{
public:
	static VegetationGeometryBuildResult succeeded(
		VegetationAssemblyGeometry geometry)
	{
		return VegetationGeometryBuildResult(std::move(geometry), {});
	}

	static VegetationGeometryBuildResult failed(std::string diagnostic)
	{
		return VegetationGeometryBuildResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return geometry_.has_value(); }
	const std::optional<VegetationAssemblyGeometry> &geometry() const
	{
		return geometry_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VegetationGeometryBuildResult(
		std::optional<VegetationAssemblyGeometry> geometry,
		std::string diagnostic)
		: geometry_(std::move(geometry)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VegetationAssemblyGeometry> geometry_;
	std::string diagnostic_;
};

