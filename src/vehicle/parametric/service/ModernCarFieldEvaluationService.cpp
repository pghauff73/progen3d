#include "vehicle/parametric/service/ModernCarFieldEvaluationService.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

double sign_of(double value)
{
	return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0);
}

double endpoint_slope(
	double first_interval,
	double second_interval,
	double first_delta,
	double second_delta)
{
	double slope = ((2.0 * first_interval + second_interval) * first_delta -
	                first_interval * second_delta) /
	               (first_interval + second_interval);
	if (sign_of(slope) != sign_of(first_delta)) return 0.0;
	if (sign_of(first_delta) != sign_of(second_delta) &&
	    std::fabs(slope) > 3.0 * std::fabs(first_delta)) {
		return 3.0 * first_delta;
	}
	return slope;
}

double smooth_minimum(double first, double second, double smoothness)
{
	const double lower = std::min(first, second);
	return lower - std::log1p(std::exp(-smoothness * std::fabs(first - second))) /
	                   smoothness;
}

double positive_power(double value, double exponent)
{
	return std::pow(std::fabs(value), exponent);
}

}

double ModernCarFieldEvaluationService::evaluateStationFunction(
	const StationFunction &function,
	double normalized_station) const
{
	const std::vector<StationFunctionKnot> &knots = function.knots();
	if (knots.empty()) return 0.0;
	if (knots.size() == 1u) return knots.front().value();
	const double station = std::clamp(normalized_station, 0.0, 1.0);
	std::size_t interval = 0u;
	while (interval + 1u < knots.size() - 1u &&
	       station > knots[interval + 1u].position()) {
		++interval;
	}

	std::vector<double> spacing(knots.size() - 1u, 0.0);
	std::vector<double> deltas(knots.size() - 1u, 0.0);
	for (std::size_t index = 0u; index + 1u < knots.size(); ++index) {
		spacing[index] = knots[index + 1u].position() - knots[index].position();
		deltas[index] =
			(knots[index + 1u].value() - knots[index].value()) / spacing[index];
	}

	std::vector<double> slopes(knots.size(), 0.0);
	if (knots.size() == 2u) {
		slopes[0] = deltas[0];
		slopes[1] = deltas[0];
	}
	else {
		slopes[0] = endpoint_slope(
			spacing[0], spacing[1], deltas[0], deltas[1]);
		for (std::size_t index = 1u; index + 1u < knots.size(); ++index) {
			if (deltas[index - 1u] == 0.0 || deltas[index] == 0.0 ||
			    sign_of(deltas[index - 1u]) != sign_of(deltas[index])) {
				slopes[index] = 0.0;
			}
			else {
				const double first_weight = 2.0 * spacing[index] + spacing[index - 1u];
				const double second_weight = spacing[index] + 2.0 * spacing[index - 1u];
				slopes[index] = (first_weight + second_weight) /
					(first_weight / deltas[index - 1u] +
					 second_weight / deltas[index]);
			}
		}
		const std::size_t last = knots.size() - 1u;
		slopes[last] = endpoint_slope(
			spacing[last - 1u], spacing[last - 2u],
			deltas[last - 1u], deltas[last - 2u]);
	}

	const double interval_length = spacing[interval];
	const double local =
		(station - knots[interval].position()) / interval_length;
	const double local_squared = local * local;
	const double local_cubed = local_squared * local;
	const double first_basis = 2.0 * local_cubed - 3.0 * local_squared + 1.0;
	const double first_slope_basis = local_cubed - 2.0 * local_squared + local;
	const double second_basis = -2.0 * local_cubed + 3.0 * local_squared;
	const double second_slope_basis = local_cubed - local_squared;
	return first_basis * knots[interval].value() +
	       first_slope_basis * interval_length * slopes[interval] +
	       second_basis * knots[interval + 1u].value() +
	       second_slope_basis * interval_length * slopes[interval + 1u];
}

double ModernCarFieldEvaluationService::normalizedBodyStation(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	return std::clamp(
		(source_x - domain.rear()) / (domain.front() - domain.rear()),
		0.0, 1.0);
}

double ModernCarFieldEvaluationService::normalizedCabinStation(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	return std::clamp(
		(source_x - domain.cabinRear()) /
			(domain.cabinFront() - domain.cabinRear()),
		0.0, 1.0);
}

double ModernCarFieldEvaluationService::asymmetricNormalized(
	double value,
	double rear,
	double middle,
	double front) const
{
	return value >= middle ? (value - middle) / (front - middle)
	                       : (middle - value) / (middle - rear);
}

double ModernCarFieldEvaluationService::bodyHalfWidth(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const LowerBodyFieldDefinition &field = variant.body().lowerBody();
	return evaluateStationFunction(field.widthRatioByStation(),
	                               normalizedBodyStation(variant, source_x)) *
	       variant.package().width() * 0.5 * field.widthScale();
}

double ModernCarFieldEvaluationService::bodyCentreHeight(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	return evaluateStationFunction(
		variant.body().lowerBody().centreHeightByStation(),
		normalizedBodyStation(variant, source_x));
}

double ModernCarFieldEvaluationService::bodyHalfHeight(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	return evaluateStationFunction(
		variant.body().lowerBody().halfHeightByStation(),
		normalizedBodyStation(variant, source_x));
}

double ModernCarFieldEvaluationService::roofHalfWidth(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const GreenhouseFieldDefinition &field = variant.body().greenhouse();
	return evaluateStationFunction(field.widthRatioByStation(),
	                               normalizedCabinStation(variant, source_x)) *
	       variant.package().width() * 0.5 * field.widthScale();
}

double ModernCarFieldEvaluationService::roofCentreHeight(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	return evaluateStationFunction(
		variant.body().greenhouse().centreHeightByStation(),
		normalizedCabinStation(variant, source_x));
}

double ModernCarFieldEvaluationService::roofHalfHeight(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	return evaluateStationFunction(
		variant.body().greenhouse().halfHeightByStation(),
		normalizedCabinStation(variant, source_x));
}

double ModernCarFieldEvaluationService::roofTop(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	const GreenhouseFieldDefinition &field = variant.body().greenhouse();
	const double normalized = std::fabs(asymmetricNormalized(
		source_x, domain.cabinRear(), domain.cabinMiddle(), domain.cabinFront()));
	const double crown = std::pow(
		std::max(0.0, 1.0 - std::pow(normalized,
			field.roofCrownLongitudinalExponent())),
		1.0 / field.roofCrownVerticalExponent());
	return roofCentreHeight(variant, source_x) +
	       roofHalfHeight(variant, source_x) * crown;
}

double ModernCarFieldEvaluationService::beltHeight(
	const ModernCarVariantDefinition &variant,
	double source_x) const
{
	const double station = normalizedCabinStation(variant, source_x);
	const double vertical_scale = variant.package().height() / 1.46812;
	return (0.82 + 0.13 * (1.0 - station) +
	        0.03 * std::sin(3.14159265358979323846 * station)) *
	       vertical_scale;
}

double ModernCarFieldEvaluationService::evaluateCabinField(
	const ModernCarVariantDefinition &variant,
	const McsM1Coordinate &source_point) const
{
	const glm::dvec3 &point = source_point.value();
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	if (point.x < domain.cabinRear() || point.x > domain.cabinFront()) return 10.0;
	const GreenhouseFieldDefinition &field = variant.body().greenhouse();
	const double longitudinal = asymmetricNormalized(
		point.x, domain.cabinRear(), domain.cabinMiddle(), domain.cabinFront());
	const double half_width = roofHalfWidth(variant, point.x);
	const double centre_height = roofCentreHeight(variant, point.x);
	const double half_height = roofHalfHeight(variant, point.x);
	return positive_power(longitudinal, field.longitudinalExponent()) +
	       positive_power(point.y / (half_width + 1.0e-6), field.lateralExponent()) +
	       positive_power(
		   (point.z - centre_height) / (half_height + 1.0e-6),
		   field.verticalExponent()) -
	       1.0;
}

double ModernCarFieldEvaluationService::evaluateFenderField(
	const ModernCarVariantDefinition &variant,
	const McsM1Coordinate &source_point,
	bool rear_axle) const
{
	const glm::dvec3 &point = source_point.value();
	const FenderFieldDefinition &field = variant.body().fenders();
	const double axle = rear_axle ? -variant.package().wheelbase() * 0.5
	                               : variant.package().wheelbase() * 0.5;
	const double longitudinal_radius = rear_axle ? field.rearLongitudinalRadius()
	                                             : field.frontLongitudinalRadius();
	const double vertical_radius = rear_axle ? field.rearVerticalRadius()
	                                         : field.frontVerticalRadius();
	return positive_power(
			(point.x - axle) / longitudinal_radius,
			field.longitudinalExponent()) +
	       positive_power(point.y / field.halfWidth(), field.lateralExponent()) +
	       positive_power(
			(point.z - field.centreHeight()) / vertical_radius,
			field.verticalExponent()) -
	       1.0;
}

double ModernCarFieldEvaluationService::evaluateBodyField(
	const ModernCarVariantDefinition &variant,
	const McsM1Coordinate &source_point) const
{
	const glm::dvec3 &point = source_point.value();
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	const LowerBodyFieldDefinition &lower_field = variant.body().lowerBody();
	const double longitudinal = asymmetricNormalized(
		point.x, domain.rear(), domain.middle(), domain.front());
	const double half_width = bodyHalfWidth(variant, point.x);
	const double centre_height = bodyCentreHeight(variant, point.x);
	const double half_height = bodyHalfHeight(variant, point.x);
	double field = positive_power(longitudinal, lower_field.longitudinalExponent()) +
	               positive_power(
			   point.y / (half_width + 1.0e-6),
			   lower_field.lateralExponent()) +
	               positive_power(
			   (point.z - centre_height) / (half_height + 1.0e-6),
			   lower_field.verticalExponent()) -
	               1.0;
	field = smooth_minimum(
		field,
		evaluateCabinField(variant, source_point),
		variant.body().blend().lowerGreenhouseSmoothness());
	field = smooth_minimum(
		field,
		evaluateFenderField(variant, source_point, false),
		variant.body().blend().fenderSmoothness());
	field = smooth_minimum(
		field,
		evaluateFenderField(variant, source_point, true),
		variant.body().blend().fenderSmoothness());

	const double wheelhouse_radius =
		variant.wheels().radius() + variant.body().wheelhouseDifference().clearance();
	for (double axle : {variant.package().wheelbase() * 0.5,
	                    -variant.package().wheelbase() * 0.5}) {
		const double wheelhouse =
			std::sqrt(
				(point.x - axle) * (point.x - axle) +
				(point.z - variant.wheels().radius()) *
					(point.z - variant.wheels().radius())) -
			wheelhouse_radius;
		field = std::max(field, -wheelhouse);
	}
	return field;
}
