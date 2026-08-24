#pragma once

#include "vehicle/mcsmv2/model/RegisteredVehicleSemanticSurface.h"
#include "vehicle/mcsmv2/model/VehicleGlassAperture.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainPartition.h"

#include <string>
#include <utility>
#include <vector>

class McsMv21SemanticGeometry
{
public:
	McsMv21SemanticGeometry(
		std::string variant_identifier,
		GeneratedPrimitiveMesh final_body_mesh,
		GeneratedPrimitiveMesh exterior_body_mesh,
		RegisteredVehicleSemanticSurface registered_surface,
		VehicleSurfaceDomainPartition domain_partition,
		std::vector<VehicleGlassAperture> glass_apertures,
		bool assurance_v2_passed)
		: variant_identifier_(std::move(variant_identifier)),
		  final_body_mesh_(std::move(final_body_mesh)),
		  exterior_body_mesh_(std::move(exterior_body_mesh)),
		  registered_surface_(std::move(registered_surface)),
		  domain_partition_(std::move(domain_partition)),
		  glass_apertures_(std::move(glass_apertures)),
		  assurance_v2_passed_(assurance_v2_passed)
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const GeneratedPrimitiveMesh &finalBodyMesh() const { return final_body_mesh_; }
	const GeneratedPrimitiveMesh &exteriorBodyMesh() const
	{
		return exterior_body_mesh_;
	}
	const RegisteredVehicleSemanticSurface &registeredSurface() const
	{
		return registered_surface_;
	}
	const VehicleSurfaceDomainPartition &domainPartition() const
	{
		return domain_partition_;
	}
	const std::vector<VehicleGlassAperture> &glassApertures() const
	{
		return glass_apertures_;
	}
	bool assuranceV2Passed() const { return assurance_v2_passed_; }

private:
	std::string variant_identifier_;
	GeneratedPrimitiveMesh final_body_mesh_{std::make_shared<Mesh>(), {}};
	GeneratedPrimitiveMesh exterior_body_mesh_{std::make_shared<Mesh>(), {}};
	RegisteredVehicleSemanticSurface registered_surface_{
		"", GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}),
		GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}),
		GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}), {}, MeshTopologyReport(),
		SemanticImplicitCorrespondenceReport(
			SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
			SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
			0.0, 0.0, 0.0, {}, false)};
	VehicleSurfaceDomainPartition domain_partition_{
		VehicleSurfaceDomainCatalog("", {}, {}),
		VehicleSurfaceOwnership({}, {}, 0u, 0u, 0.0, 0u), {},
		0.0, 0.0, 0.0, 0.0, 0u, 0u};
	std::vector<VehicleGlassAperture> glass_apertures_;
	bool assurance_v2_passed_ = false;
};
