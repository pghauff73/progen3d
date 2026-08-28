#pragma once

#include "vehicle/parametric/model/ModernCarCoordinateFrame.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

class ModernCarFieldEvaluationService
{
public:
	double evaluateStationFunction(
		const StationFunction &function,
		double normalized_station) const;

	double evaluateBodyField(
		const ModernCarVariantDefinition &variant,
		const McsM1Coordinate &source_point) const;

	double bodyHalfWidth(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double bodyCentreHeight(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double bodyHalfHeight(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double roofHalfWidth(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double roofCentreHeight(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double roofHalfHeight(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double roofTop(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double beltHeight(
		const ModernCarVariantDefinition &variant,
		double source_x) const;

private:
	double normalizedBodyStation(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double normalizedCabinStation(
		const ModernCarVariantDefinition &variant,
		double source_x) const;
	double asymmetricNormalized(
		double value,
		double rear,
		double middle,
		double front) const;
	double evaluateCabinField(
		const ModernCarVariantDefinition &variant,
		const McsM1Coordinate &source_point) const;
	double evaluateFenderField(
		const ModernCarVariantDefinition &variant,
		const McsM1Coordinate &source_point,
		bool rear_axle) const;
};
