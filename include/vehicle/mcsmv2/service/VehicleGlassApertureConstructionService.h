#pragma once

#include "Mesh.h"
#include "vehicle/mcsmv2/model/RegisteredVehicleSemanticSurface.h"
#include "vehicle/mcsmv2/model/VehicleGlassAperture.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainCatalog.h"

#include <vector>

class VehicleGlassApertureConstructionService
{
public:
	std::vector<VehicleGlassAperture> construct(
		const RegisteredVehicleSemanticSurface &registered_surface,
		const VehicleSurfaceDomainCatalog &domain_catalog,
		const Mesh &final_body_surface,
		double outward_offset = 0.0035) const;
};
