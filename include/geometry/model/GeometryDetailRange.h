#pragma once

#include "geometry/model/GeometryDetailLevel.h"

class GeometryDetailRange
{
public:
	GeometryDetailRange(
		GeometryDetailLevel minimum = GeometryDetailLevel::Bounds,
		GeometryDetailLevel maximum = GeometryDetailLevel::FastenersAndSeals)
		: minimum_(minimum),
		  maximum_(maximum)
	{
	}

	static GeometryDetailRange exact(GeometryDetailLevel detail_level)
	{
		return GeometryDetailRange(detail_level, detail_level);
	}

	GeometryDetailLevel minimum() const { return minimum_; }
	GeometryDetailLevel maximum() const { return maximum_; }

	bool isValid() const
	{
		return geometryDetailLevelRank(minimum_) <=
		       geometryDetailLevelRank(maximum_);
	}

	bool includes(GeometryDetailLevel detail_level) const
	{
		const int rank = geometryDetailLevelRank(detail_level);
		return isValid() && rank >= geometryDetailLevelRank(minimum_) &&
		       rank <= geometryDetailLevelRank(maximum_);
	}

private:
	GeometryDetailLevel minimum_ = GeometryDetailLevel::Bounds;
	GeometryDetailLevel maximum_ = GeometryDetailLevel::FastenersAndSeals;
};
