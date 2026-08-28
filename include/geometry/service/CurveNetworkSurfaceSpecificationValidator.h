#pragma once

#include "geometry/model/CurveNetworkSurfaceShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>
#include <vector>

struct CurveNetworkSurfaceShapeSpecificationCandidate
{
	std::vector<Curve3D> constant_u_curves;
	std::vector<Curve3D> constant_v_curves;
	double intersection_tolerance = 1.0e-4;
	int samples_u = 24;
	int samples_v = 24;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class CurveNetworkSurfaceSpecificationValidator
{
public:
	explicit CurveNetworkSurfaceSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const CurveNetworkSurfaceShapeSpecification> validate(
		CurveNetworkSurfaceShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
