#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/LoftSectionSpecification.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>
#include <vector>

class LoftShapeSpecification : public ShapeSpecification
{
public:
	LoftShapeSpecification(
		std::vector<LoftSectionSpecification> sections,
		ExtrudeProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text,
		ShapeFamily family = ShapeFamily::Loft,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(family, std::move(key), detail_level),
		  sections_(std::move(sections)),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const std::vector<LoftSectionSpecification> &sections() const
	{
		return sections_;
	}
	const ExtrudeProfileCapPolicy &capPolicy() const { return cap_policy_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return cap_policy_.closesBothEnds(); }

private:
	std::vector<LoftSectionSpecification> sections_;
	ExtrudeProfileCapPolicy cap_policy_ = ExtrudeProfileCapPolicy::createAll();
	std::string canonical_text_;
};
