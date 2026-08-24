#include "vehicle/parametric/service/ModernCarCharacterCurveExtractionService.h"

#include "vehicle/parametric/service/ModernCarCoordinateConversionService.h"
#include "vehicle/parametric/service/ModernCarFieldEvaluationService.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

std::vector<double> evenly_spaced(double minimum, double maximum, std::size_t count)
{
	std::vector<double> values;
	values.reserve(count);
	if (count == 0u) return values;
	if (count == 1u) return {minimum};
	for (std::size_t index = 0u; index < count; ++index) {
		values.push_back(
			minimum + (maximum - minimum) *
				static_cast<double>(index) / static_cast<double>(count - 1u));
	}
	return values;
}

} // namespace

GeneratedCharacterCurveSet ModernCarCharacterCurveExtractionService::extract(
	const ModernCarVariantDefinition &variant) const
{
	const std::size_t sample_count =
		variant.body().characterCurveNetwork().samplesPerCurve();
	const VehicleLongitudinalDomain &domain = variant.body().longitudinalDomain();
	const std::vector<double> body_stations = evenly_spaced(
		domain.rear() + 0.03, domain.front() - 0.03, sample_count);
	const std::vector<double> cabin_stations = evenly_spaced(
		domain.cabinRear(), domain.cabinFront(), sample_count);
	ModernCarFieldEvaluationService field_service;
	ModernCarCoordinateConversionService coordinate_service;
	ModernCarCoordinateFrame coordinate_frame;

	auto convert_curve = [&](const std::string &identifier,
	                         const std::vector<glm::dvec3> &source_points) {
		std::vector<glm::dvec3> points;
		points.reserve(source_points.size());
		for (const glm::dvec3 &source_point : source_points) {
			points.push_back(coordinate_service.convertSourceToMcp(
				coordinate_frame, variant, McsM1Coordinate(source_point)).value());
		}
		return GeneratedCharacterCurve(identifier, std::move(points));
	};

	std::vector<glm::dvec3> centre_spine;
	std::vector<glm::dvec3> shoulder_left;
	std::vector<glm::dvec3> shoulder_right;
	std::vector<glm::dvec3> rocker_left;
	std::vector<glm::dvec3> rocker_right;
	for (double station : body_stations) {
		const double half_width = field_service.bodyHalfWidth(variant, station);
		const double centre_height = field_service.bodyCentreHeight(variant, station);
		const double half_height = field_service.bodyHalfHeight(variant, station);
		const double shoulder_height = centre_height + half_height * 0.47;
		const double rocker_height =
			0.20 * variant.package().height() / 1.46812;
		centre_spine.emplace_back(station, 0.0, centre_height);
		shoulder_left.emplace_back(station, -half_width, shoulder_height);
		shoulder_right.emplace_back(station, half_width, shoulder_height);
		rocker_left.emplace_back(station, -half_width * 0.94, rocker_height);
		rocker_right.emplace_back(station, half_width * 0.94, rocker_height);
	}

	std::vector<glm::dvec3> roof_centre;
	std::vector<glm::dvec3> belt_left;
	std::vector<glm::dvec3> belt_right;
	for (double station : cabin_stations) {
		const double belt_width =
			field_service.bodyHalfWidth(variant, station) * 0.93;
		const double belt_height = field_service.beltHeight(variant, station);
		roof_centre.emplace_back(
			station, 0.0, field_service.roofTop(variant, station));
		belt_left.emplace_back(station, -belt_width, belt_height);
		belt_right.emplace_back(station, belt_width, belt_height);
	}

	std::vector<GeneratedCharacterCurve> curves;
	curves.reserve(8u);
	curves.push_back(convert_curve("centre_spine", centre_spine));
	curves.push_back(convert_curve("roof_centre", roof_centre));
	curves.push_back(convert_curve("belt_left", belt_left));
	curves.push_back(convert_curve("belt_right", belt_right));
	curves.push_back(convert_curve("shoulder_left", shoulder_left));
	curves.push_back(convert_curve("shoulder_right", shoulder_right));
	curves.push_back(convert_curve("rocker_left", rocker_left));
	curves.push_back(convert_curve("rocker_right", rocker_right));
	return GeneratedCharacterCurveSet(std::move(curves));
}
