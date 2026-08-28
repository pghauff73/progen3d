#pragma once

#include "geometry/model/LoftShapeSpecification.h"

class SurfaceLoftShapeSpecification : public LoftShapeSpecification
{
public:
	SurfaceLoftShapeSpecification(
		std::vector<LoftSectionSpecification> sections,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: LoftShapeSpecification(
			std::move(sections),
			ExtrudeProfileCapPolicy::createNone(),
			std::move(key),
			std::move(canonical_text),
			ShapeFamily::SurfaceLoft,
			detail_level)
	{
	}
};
