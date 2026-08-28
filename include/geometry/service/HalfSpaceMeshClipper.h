#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/PlaneClipSpecification.h"

#include <cstddef>
#include <string>

class HalfSpaceMeshClipper
{
public:
	GeneratedPrimitiveMesh clip(
		const GeneratedPrimitiveMesh &source,
		const PlaneClipSpecification &clip_specification,
		float outer_radius,
		std::size_t clip_index,
		bool close_boundary,
		std::string *diagnostic) const;
};
