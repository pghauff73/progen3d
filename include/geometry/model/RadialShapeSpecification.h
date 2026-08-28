#pragma once

#include "geometry/model/PlaneClipSpecification.h"
#include "geometry/model/RadialDomainSpecification.h"
#include "geometry/model/ShapeClosurePolicy.h"
#include "geometry/model/ShapeSpecification.h"
#include "geometry/model/ShapeTessellation.h"
#include "geometry/model/ShapeTopology.h"

#include <utility>
#include <vector>

enum class ShapeMappingMode
{
	Triplanar,
	Cylindrical,
	Spherical
};

class RadialShapeSpecification : public ShapeSpecification
{
public:
	RadialShapeSpecification(ShapeFamily family,
	                         RadialDomainSpecification radial_domain,
	                         ShapeTopology topology,
	                         ShapeClosurePolicy closure_policy,
	                         ShapeTessellation tessellation,
	                         ShapeMappingMode mapping_mode,
	                         std::vector<PlaneClipSpecification> clips,
	                         ShapeSpecificationKey key)
		: ShapeSpecification(family, std::move(key)),
		  radial_domain_(radial_domain),
		  topology_(topology),
		  closure_policy_(closure_policy),
		  tessellation_(tessellation),
		  mapping_mode_(mapping_mode),
		  clips_(std::move(clips))
	{
	}

	const RadialDomainSpecification &radialDomain() const
	{
		return radial_domain_;
	}

	ShapeTopology topology() const
	{
		return topology_;
	}

	const ShapeClosurePolicy &closurePolicy() const
	{
		return closure_policy_;
	}

	const ShapeTessellation &tessellation() const
	{
		return tessellation_;
	}

	ShapeMappingMode mappingMode() const
	{
		return mapping_mode_;
	}

	const std::vector<PlaneClipSpecification> &clips() const
	{
		return clips_;
	}

private:
	RadialDomainSpecification radial_domain_;
	ShapeTopology topology_;
	ShapeClosurePolicy closure_policy_;
	ShapeTessellation tessellation_;
	ShapeMappingMode mapping_mode_;
	std::vector<PlaneClipSpecification> clips_;
};
