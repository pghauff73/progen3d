#pragma once

#include "architecture/model/LayerSetGeometry.h"
#include "geometry/model/GeometryBuildStatus.h"

#include <optional>
#include <string>
#include <utility>

class LayerSetBuildResult
{
public:
	static LayerSetBuildResult succeeded(LayerSetGeometry geometry)
	{
		return LayerSetBuildResult(
			GeometryBuildStatus::Success, std::move(geometry), {});
	}

	static LayerSetBuildResult failed(
		GeometryBuildStatus status,
		std::string diagnostic)
	{
		return LayerSetBuildResult(status, std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return geometry_.has_value(); }
	GeometryBuildStatus status() const { return status_; }
	const std::optional<LayerSetGeometry> &geometry() const { return geometry_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	LayerSetBuildResult(GeometryBuildStatus status,
	                    std::optional<LayerSetGeometry> geometry,
	                    std::string diagnostic)
		: status_(status),
		  geometry_(std::move(geometry)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	GeometryBuildStatus status_ = GeometryBuildStatus::UnsupportedTopology;
	std::optional<LayerSetGeometry> geometry_;
	std::string diagnostic_;
};
