#pragma once

#include "geometry/model/LoftShapeSpecification.h"
#include "geometry/model/ShellOffsetShapeSpecification.h"

class FormedPanelShapeSpecification : public LoftShapeSpecification
{
public:
	FormedPanelShapeSpecification(
		std::vector<LoftSectionSpecification> sections,
		float thickness,
		ShellOffsetSide offset_side,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: LoftShapeSpecification(
			std::move(sections),
			ExtrudeProfileCapPolicy::createNone(),
			std::move(key),
			std::move(canonical_text),
			ShapeFamily::FormedPanel,
			detail_level),
		  thickness_(thickness),
		  offset_side_(offset_side)
	{
	}

	float thickness() const { return thickness_; }
	ShellOffsetSide offsetSide() const { return offset_side_; }
	bool requestsClosedGeometry() const override { return true; }

private:
	float thickness_ = 0.0f;
	ShellOffsetSide offset_side_ = ShellOffsetSide::Both;
};
