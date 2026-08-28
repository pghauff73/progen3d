#pragma once

#include "vehicle/mcsmv2/model/VehicleSurfaceDomain.h"

#include <string>
#include <utility>
#include <vector>

class VehicleSurfaceDomainCatalog
{
public:
	VehicleSurfaceDomainCatalog(
		std::string variant_identifier,
		std::vector<VehicleSurfaceDomain> panel_domains,
		std::vector<VehicleSurfaceDomain> aperture_domains)
		: variant_identifier_(std::move(variant_identifier)),
		  panel_domains_(std::move(panel_domains)),
		  aperture_domains_(std::move(aperture_domains))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<VehicleSurfaceDomain> &panelDomains() const
	{
		return panel_domains_;
	}
	const std::vector<VehicleSurfaceDomain> &apertureDomains() const
	{
		return aperture_domains_;
	}

private:
	std::string variant_identifier_;
	std::vector<VehicleSurfaceDomain> panel_domains_;
	std::vector<VehicleSurfaceDomain> aperture_domains_;
};
