#pragma once

#include "vehicle/mcsmv2/model/VehiclePanelAdjacencyRelationship.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceDomainCatalog.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceOwnership.h"

#include <utility>
#include <vector>

class VehicleSurfaceDomainPartition
{
public:
	VehicleSurfaceDomainPartition(
		VehicleSurfaceDomainCatalog domain_catalog,
		VehicleSurfaceOwnership ownership,
		std::vector<VehiclePanelAdjacencyRelationship> panel_adjacencies,
		double panel_domain_coverage_fraction,
		double aperture_domain_coverage_fraction,
		double derived_fixed_body_coverage_fraction,
		double declared_exterior_coverage_fraction,
		std::size_t panel_overlap_sample_count,
		std::size_t aperture_overlap_sample_count)
		: domain_catalog_(std::move(domain_catalog)),
		  ownership_(std::move(ownership)),
		  panel_adjacencies_(std::move(panel_adjacencies)),
		  panel_domain_coverage_fraction_(panel_domain_coverage_fraction),
		  aperture_domain_coverage_fraction_(aperture_domain_coverage_fraction),
		  derived_fixed_body_coverage_fraction_(
			  derived_fixed_body_coverage_fraction),
		  declared_exterior_coverage_fraction_(declared_exterior_coverage_fraction),
		  panel_overlap_sample_count_(panel_overlap_sample_count),
		  aperture_overlap_sample_count_(aperture_overlap_sample_count)
	{
	}

	const VehicleSurfaceDomainCatalog &domainCatalog() const { return domain_catalog_; }
	const VehicleSurfaceOwnership &ownership() const { return ownership_; }
	const std::vector<VehiclePanelAdjacencyRelationship> &panelAdjacencies() const
	{
		return panel_adjacencies_;
	}
	double panelDomainCoverageFraction() const
	{
		return panel_domain_coverage_fraction_;
	}
	double apertureDomainCoverageFraction() const
	{
		return aperture_domain_coverage_fraction_;
	}
	double derivedFixedBodyCoverageFraction() const
	{
		return derived_fixed_body_coverage_fraction_;
	}
	double declaredExteriorCoverageFraction() const
	{
		return declared_exterior_coverage_fraction_;
	}
	std::size_t panelOverlapSampleCount() const { return panel_overlap_sample_count_; }
	std::size_t apertureOverlapSampleCount() const
	{
		return aperture_overlap_sample_count_;
	}
	bool passed() const
	{
		return ownership_.passed() && panel_overlap_sample_count_ == 0u &&
		       aperture_overlap_sample_count_ == 0u &&
		       declared_exterior_coverage_fraction_ >= 1.0;
	}

private:
	VehicleSurfaceDomainCatalog domain_catalog_{"", {}, {}};
	VehicleSurfaceOwnership ownership_{{}, {}, 0u, 0u, 0.0, 0u};
	std::vector<VehiclePanelAdjacencyRelationship> panel_adjacencies_;
	double panel_domain_coverage_fraction_ = 0.0;
	double aperture_domain_coverage_fraction_ = 0.0;
	double derived_fixed_body_coverage_fraction_ = 0.0;
	double declared_exterior_coverage_fraction_ = 0.0;
	std::size_t panel_overlap_sample_count_ = 0u;
	std::size_t aperture_overlap_sample_count_ = 0u;
};
