#pragma once

#include "geometry/model/CurveNetworkSurface.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>
#include <utility>

class CurveNetworkSurfaceShapeSpecification : public ShapeSpecification
{
public:
	CurveNetworkSurfaceShapeSpecification(
		CurveNetworkSurface surface,
		int samples_u,
		int samples_v,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals)
		: ShapeSpecification(
			ShapeFamily::CurveNetworkSurface, std::move(key), detail_level),
		  surface_(std::move(surface)),
		  samples_u_(samples_u),
		  samples_v_(samples_v),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const CurveNetworkSurface &surface() const { return surface_; }
	int samplesU() const { return samples_u_; }
	int samplesV() const { return samples_v_; }

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	CurveNetworkSurface surface_;
	int samples_u_ = 24;
	int samples_v_ = 24;
	std::string canonical_text_;
};
