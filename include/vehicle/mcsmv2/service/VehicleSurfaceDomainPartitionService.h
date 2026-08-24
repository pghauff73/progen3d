#pragma once

#include "Mesh.h"
#include "vehicle/mcsmv2/model/RegisteredVehicleSemanticSurface.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainCatalog.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainPartition.h"

#include <cstddef>

class VehicleSurfaceDomainPartitionService
{
public:
	VehicleSurfaceDomainPartition partition(
		const Mesh &final_body_surface,
		const RegisteredVehicleSemanticSurface &registered_surface,
		const VehicleSurfaceDomainCatalog &domain_catalog,
		std::size_t domain_grid_samples = 401u,
		double exterior_mapping_limit = 0.055,
		double aperture_mapping_limit = 0.085) const;
};
