#pragma once

#include "vehicle/mcsmv2/model/ShapePreservingCubicInterpolation.h"
#include "vehicle/mcsmv2/model/VehicleSectionScalarField.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSectionField.h"

#include <array>
#include <vector>

class VehicleSemanticSectionFieldEvaluationService
{
public:
	explicit VehicleSemanticSectionFieldEvaluationService(
		const VehicleSemanticSectionField &section_field);

	double evaluate(
		VehicleSectionScalarField scalar_field,
		double source_x) const;
	double halfWidthAt(double source_x, double source_height) const;
	std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> evaluateLandmarks(
		double source_x) const;
	double rearSourceX() const { return rear_source_x_; }
	double frontSourceX() const { return front_source_x_; }

private:
	double evaluateVerticalHeight(
		VehicleSectionScalarField scalar_field,
		double source_x) const;
	double evaluateHalfWidth(
		VehicleSectionScalarField scalar_field,
		double source_x) const;

	std::vector<ShapePreservingCubicInterpolation> vertical_height_interpolations_;
	std::vector<ShapePreservingCubicInterpolation> half_width_interpolations_;
	double rear_source_x_ = 0.0;
	double front_source_x_ = 0.0;
};
