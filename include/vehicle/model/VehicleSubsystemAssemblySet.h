#pragma once

#include "vehicle/model/ModernVehicleAssembly.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class VehicleSubsystemAssemblySet
{
public:
	explicit VehicleSubsystemAssemblySet(
		std::vector<VehiclePlacedAssembly> assemblies)
		: assemblies_(std::move(assemblies))
	{
	}

	const std::vector<VehiclePlacedAssembly> &assemblies() const
	{
		return assemblies_;
	}

private:
	std::vector<VehiclePlacedAssembly> assemblies_;
};

class VehicleSubsystemAssemblyBuildResult
{
public:
	static VehicleSubsystemAssemblyBuildResult succeeded(
		VehicleSubsystemAssemblySet assembly_set)
	{
		return VehicleSubsystemAssemblyBuildResult(
			std::move(assembly_set), {});
	}

	static VehicleSubsystemAssemblyBuildResult failed(std::string diagnostic)
	{
		return VehicleSubsystemAssemblyBuildResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return assembly_set_.has_value(); }
	const std::optional<VehicleSubsystemAssemblySet> &assemblySet() const
	{
		return assembly_set_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VehicleSubsystemAssemblyBuildResult(
		std::optional<VehicleSubsystemAssemblySet> assembly_set,
		std::string diagnostic)
		: assembly_set_(std::move(assembly_set)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VehicleSubsystemAssemblySet> assembly_set_;
	std::string diagnostic_;
};
