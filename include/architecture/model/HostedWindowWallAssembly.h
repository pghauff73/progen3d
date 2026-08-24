#pragma once

#include "architecture/model/ArchitecturalAssemblyGeometry.h"
#include "architecture/model/OpeningBoundaryPlacementResult.h"

#include <utility>

class HostedWindowWallAssembly
{
public:
	HostedWindowWallAssembly(
		ArchitecturalAssemblyGeometry geometry,
		OpeningBoundaryPlacementResult placement)
		: geometry_(std::move(geometry)),
		  placement_(std::move(placement))
	{
	}

	const ArchitecturalAssemblyGeometry &geometry() const { return geometry_; }
	const OpeningBoundaryPlacementResult &placement() const { return placement_; }

private:
	ArchitecturalAssemblyGeometry geometry_{{},
		GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}), {}};
	OpeningBoundaryPlacementResult placement_ =
		OpeningBoundaryPlacementResult::failed("Unresolved hosted window placement.");
};
