#pragma once

#include "geometry/model/Curve3D.h"
#include "geometry/model/Surface3D.h"

#include <vector>

class CurveNetworkSurface : public Surface3D
{
public:
	CurveNetworkSurface(
		std::vector<Curve3D> constant_u_curves,
		std::vector<Curve3D> constant_v_curves,
		double intersection_tolerance);

	bool isValid(std::string *diagnostic = nullptr) const override;
	glm::dvec3 evaluatePosition(double u, double v) const override;

	const std::vector<Curve3D> &constantUCurves() const;
	const std::vector<Curve3D> &constantVCurves() const;
	double intersectionTolerance() const;

private:
	static std::vector<double> localLinearBasis(std::size_t count, double parameter);
	glm::dvec3 intersectionPoint(std::size_t u_index, std::size_t v_index) const;

	std::vector<Curve3D> constant_u_curves_;
	std::vector<Curve3D> constant_v_curves_;
	double intersection_tolerance_ = 1.0e-4;
};
