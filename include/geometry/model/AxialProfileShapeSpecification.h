#pragma once

#include "geometry/model/AxialProfileAxis.h"
#include "geometry/model/AxialProfileCapPolicy.h"
#include "geometry/model/AxialProfileLevel.h"
#include "geometry/model/AxialProfilePolygon.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>
#include <vector>

class AxialProfileShapeSpecification : public ShapeSpecification
{
public:
	AxialProfileShapeSpecification(
		AxialProfileAxis axis,
		std::vector<AxialProfilePolygon> profiles,
		std::vector<AxialProfileLevel> levels,
		AxialProfileCapPolicy cap_policy,
		ShapeSpecificationKey key,
		std::string canonical_text)
		: ShapeSpecification(ShapeFamily::AxialProfile, std::move(key)),
		  axis_(axis),
		  profiles_(std::move(profiles)),
		  levels_(std::move(levels)),
		  cap_policy_(cap_policy),
		  canonical_text_(std::move(canonical_text))
	{
	}

	AxialProfileAxis axis() const
	{
		return axis_;
	}

	const std::vector<AxialProfilePolygon> &profiles() const
	{
		return profiles_;
	}

	const std::vector<AxialProfileLevel> &levels() const
	{
		return levels_;
	}

	const AxialProfileCapPolicy &capPolicy() const
	{
		return cap_policy_;
	}

	std::string canonicalText() const override
	{
		return canonical_text_;
	}

	bool isDefaultFamilyShape() const override
	{
		return false;
	}

	bool requestsClosedGeometry() const override
	{
		return cap_policy_.closesBothEnds();
	}

private:
	AxialProfileAxis axis_ = AxialProfileAxis::Y;
	std::vector<AxialProfilePolygon> profiles_;
	std::vector<AxialProfileLevel> levels_;
	AxialProfileCapPolicy cap_policy_ = AxialProfileCapPolicy::createAll();
	std::string canonical_text_;
};
