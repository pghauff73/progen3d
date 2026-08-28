#pragma once

#include "architecture/model/HostedWindowWallAssembly.h"
#include "geometry/model/GeometryBuildStatus.h"

#include <optional>
#include <string>
#include <utility>

class HostedWindowWallAssemblyBuildResult
{
public:
	static HostedWindowWallAssemblyBuildResult succeeded(
		HostedWindowWallAssembly assembly)
	{
		return HostedWindowWallAssemblyBuildResult(
			GeometryBuildStatus::Success, std::move(assembly), {});
	}

	static HostedWindowWallAssemblyBuildResult failed(
		GeometryBuildStatus status,
		std::string diagnostic)
	{
		return HostedWindowWallAssemblyBuildResult(
			status, std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return assembly_.has_value(); }
	GeometryBuildStatus status() const { return status_; }
	const std::optional<HostedWindowWallAssembly> &assembly() const
	{
		return assembly_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	HostedWindowWallAssemblyBuildResult(
		GeometryBuildStatus status,
		std::optional<HostedWindowWallAssembly> assembly,
		std::string diagnostic)
		: status_(status),
		  assembly_(std::move(assembly)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	GeometryBuildStatus status_ = GeometryBuildStatus::UnsupportedTopology;
	std::optional<HostedWindowWallAssembly> assembly_;
	std::string diagnostic_;
};
