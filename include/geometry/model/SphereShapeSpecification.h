#pragma once

#include "geometry/model/AngularDomainSpecification.h"
#include "geometry/model/PolarDomainSpecification.h"
#include "geometry/model/RadialShapeSpecification.h"

class SphereShapeSpecification : public RadialShapeSpecification
{
public:
	SphereShapeSpecification(RadialDomainSpecification radial_domain,
	                         PolarDomainSpecification polar_domain,
	                         AngularDomainSpecification angular_domain,
	                         ShapeTopology topology,
	                         ShapeClosurePolicy closure_policy,
	                         ShapeTessellation tessellation,
	                         ShapeMappingMode mapping_mode,
	                         std::vector<PlaneClipSpecification> clips,
	                         ShapeSpecificationKey key,
	                         std::string canonical_text,
	                         bool default_family_shape)
		: RadialShapeSpecification(ShapeFamily::Sphere,
		                           radial_domain,
		                           topology,
		                           closure_policy,
		                           tessellation,
		                           mapping_mode,
		                           std::move(clips),
		                           std::move(key)),
		  polar_domain_(polar_domain),
		  angular_domain_(angular_domain),
		  canonical_text_(std::move(canonical_text)),
		  default_family_shape_(default_family_shape)
	{
	}

	const PolarDomainSpecification &polarDomain() const
	{
		return polar_domain_;
	}

	const AngularDomainSpecification &angularDomain() const
	{
		return angular_domain_;
	}

	std::string canonicalText() const override
	{
		return canonical_text_;
	}

	bool isDefaultFamilyShape() const override
	{
		return default_family_shape_;
	}

	bool requestsClosedGeometry() const override
	{
		if (topology() == ShapeTopology::Surface) return false;
		if (!clips().empty() && !closurePolicy().closesClipBoundaries()) return false;
		const bool has_polar_boundary = polar_domain_.minimumDegrees() > 0.0f ||
		                                polar_domain_.maximumDegrees() < 180.0f;
		if (has_polar_boundary && !closurePolicy().closesRims()) return false;
		return angular_domain_.isFullRevolution() ||
		       closurePolicy().closesAngularBoundaries() ||
		       closurePolicy().closesRims();
	}

private:
	PolarDomainSpecification polar_domain_;
	AngularDomainSpecification angular_domain_;
	std::string canonical_text_;
	bool default_family_shape_ = false;
};
