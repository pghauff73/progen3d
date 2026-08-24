#pragma once

#include "geometry/model/BinarySilhouette.h"
#include "geometry/model/OrthographicProjectionView.h"

#include <utility>

class ThreeViewProjection
{
public:
	ThreeViewProjection(BinarySilhouette side,
	                    BinarySilhouette front,
	                    BinarySilhouette top)
		: side_(std::move(side)),
		  front_(std::move(front)),
		  top_(std::move(top))
	{
	}

	const BinarySilhouette &silhouette(OrthographicProjectionView view) const
	{
		switch (view) {
		case OrthographicProjectionView::Side: return side_;
		case OrthographicProjectionView::Front: return front_;
		case OrthographicProjectionView::Top: return top_;
		}
		return side_;
	}

private:
	BinarySilhouette side_;
	BinarySilhouette front_;
	BinarySilhouette top_;
};
