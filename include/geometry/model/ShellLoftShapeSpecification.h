#pragma once

#include "geometry/model/LoftShapeSpecification.h"

class ShellLoftShapeSpecification : public LoftShapeSpecification
{
public:
	ShellLoftShapeSpecification(
		std::vector<LoftSectionSpecification> sections,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: LoftShapeSpecification(
			std::move(sections),
			cap_policy,
			std::move(key),
			std::move(canonical_text),
			ShapeFamily::ShellLoft,
			detail_level)
	{
	}
};
