#pragma once

#include "architecture/model/ArchitecturalAssemblyGeometry.h"
#include "geometry/model/GeometryBuildStatus.h"

#include <optional>
#include <string>
#include <utility>

class ArchitecturalAssemblyBuildResult
{
public:
	static ArchitecturalAssemblyBuildResult succeeded(
		ArchitecturalAssemblyGeometry geometry)
	{
		return ArchitecturalAssemblyBuildResult(
			GeometryBuildStatus::Success, std::move(geometry), {});
	}

	static ArchitecturalAssemblyBuildResult failed(
		GeometryBuildStatus status,
		std::string diagnostic)
	{
		return ArchitecturalAssemblyBuildResult(
			status, std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return geometry_.has_value(); }
	GeometryBuildStatus status() const { return status_; }
	const std::optional<ArchitecturalAssemblyGeometry> &geometry() const
	{
		return geometry_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	ArchitecturalAssemblyBuildResult(
		GeometryBuildStatus status,
		std::optional<ArchitecturalAssemblyGeometry> geometry,
		std::string diagnostic)
		: status_(status),
		  geometry_(std::move(geometry)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	GeometryBuildStatus status_ = GeometryBuildStatus::UnsupportedTopology;
	std::optional<ArchitecturalAssemblyGeometry> geometry_;
	std::string diagnostic_;
};
