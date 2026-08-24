#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomain.h"

#include <string>
#include <utility>

class VehicleGlassAperture
{
public:
	VehicleGlassAperture(
		std::string identifier,
		VehicleSurfaceDomain aperture_domain,
		GeneratedPrimitiveMesh support_mesh,
		GeneratedPrimitiveMesh glass_mesh,
		double outward_offset,
		double support_area,
		double minimum_body_separation,
		double maximum_body_separation,
		bool passed)
		: identifier_(std::move(identifier)),
		  aperture_domain_(std::move(aperture_domain)),
		  support_mesh_(std::move(support_mesh)),
		  glass_mesh_(std::move(glass_mesh)),
		  outward_offset_(outward_offset),
		  support_area_(support_area),
		  minimum_body_separation_(minimum_body_separation),
		  maximum_body_separation_(maximum_body_separation),
		  passed_(passed)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleSurfaceDomain &apertureDomain() const { return aperture_domain_; }
	const GeneratedPrimitiveMesh &supportMesh() const { return support_mesh_; }
	const GeneratedPrimitiveMesh &glassMesh() const { return glass_mesh_; }
	double outwardOffset() const { return outward_offset_; }
	double supportArea() const { return support_area_; }
	double minimumBodySeparation() const { return minimum_body_separation_; }
	double maximumBodySeparation() const { return maximum_body_separation_; }
	bool passed() const { return passed_; }

private:
	std::string identifier_;
	VehicleSurfaceDomain aperture_domain_{"", VehicleSurfaceDomainKind::Aperture, false, "", {}};
	GeneratedPrimitiveMesh support_mesh_{std::make_shared<Mesh>(), {}};
	GeneratedPrimitiveMesh glass_mesh_{std::make_shared<Mesh>(), {}};
	double outward_offset_ = 0.0;
	double support_area_ = 0.0;
	double minimum_body_separation_ = 0.0;
	double maximum_body_separation_ = 0.0;
	bool passed_ = false;
};
