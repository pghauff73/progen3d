#include "vehicle/mcsmv2/service/VehicleSemanticSectionFieldEvaluationService.h"

#include "vehicle/mcsmv2/service/VehicleSemanticSectionInterpolationService.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

double scalarValue(
	const VehicleSemanticSectionStation &station,
	VehicleSectionScalarField field)
{
	switch (field) {
	case VehicleSectionScalarField::UnderbodyHeight:
		return station.landmark(VehicleSectionLandmarkRole::Underbody).height();
	case VehicleSectionScalarField::UnderbodyHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::Underbody).halfWidth();
	case VehicleSectionScalarField::RockerHeight:
		return station.landmark(VehicleSectionLandmarkRole::Rocker).height();
	case VehicleSectionScalarField::RockerHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::Rocker).halfWidth();
	case VehicleSectionScalarField::LowerBodyHeight:
		return station.landmark(VehicleSectionLandmarkRole::LowerBody).height();
	case VehicleSectionScalarField::LowerBodyHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::LowerBody).halfWidth();
	case VehicleSectionScalarField::ShoulderHeight:
		return station.landmark(VehicleSectionLandmarkRole::Shoulder).height();
	case VehicleSectionScalarField::ShoulderHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::Shoulder).halfWidth();
	case VehicleSectionScalarField::BeltHeight:
		return station.landmark(VehicleSectionLandmarkRole::Belt).height();
	case VehicleSectionScalarField::BeltHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::Belt).halfWidth();
	case VehicleSectionScalarField::GlassShoulderHeight:
		return station.landmark(VehicleSectionLandmarkRole::GlassShoulder).height();
	case VehicleSectionScalarField::GlassShoulderHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::GlassShoulder).halfWidth();
	case VehicleSectionScalarField::RoofRailHeight:
		return station.landmark(VehicleSectionLandmarkRole::RoofRail).height();
	case VehicleSectionScalarField::RoofRailHalfWidth:
		return station.landmark(VehicleSectionLandmarkRole::RoofRail).halfWidth();
	case VehicleSectionScalarField::RoofCrownHeight:
		return station.landmark(VehicleSectionLandmarkRole::RoofCrown).height();
	}
	return 0.0;
}

const std::array<VehicleSectionScalarField, 8> kVerticalHeightFields{{
	VehicleSectionScalarField::UnderbodyHeight,
	VehicleSectionScalarField::RockerHeight,
	VehicleSectionScalarField::LowerBodyHeight,
	VehicleSectionScalarField::ShoulderHeight,
	VehicleSectionScalarField::BeltHeight,
	VehicleSectionScalarField::GlassShoulderHeight,
	VehicleSectionScalarField::RoofRailHeight,
	VehicleSectionScalarField::RoofCrownHeight}};

const std::array<VehicleSectionScalarField, 7> kHalfWidthFields{{
	VehicleSectionScalarField::UnderbodyHalfWidth,
	VehicleSectionScalarField::RockerHalfWidth,
	VehicleSectionScalarField::LowerBodyHalfWidth,
	VehicleSectionScalarField::ShoulderHalfWidth,
	VehicleSectionScalarField::BeltHalfWidth,
	VehicleSectionScalarField::GlassShoulderHalfWidth,
	VehicleSectionScalarField::RoofRailHalfWidth}};

std::size_t fieldIndex(
	VehicleSectionScalarField scalar_field,
	const std::array<VehicleSectionScalarField, 8> &fields)
{
	const auto found = std::find(fields.begin(), fields.end(), scalar_field);
	if (found == fields.end()) {
		throw std::invalid_argument("Requested field is not a vertical height field.");
	}
	return static_cast<std::size_t>(std::distance(fields.begin(), found));
}

std::size_t fieldIndex(
	VehicleSectionScalarField scalar_field,
	const std::array<VehicleSectionScalarField, 7> &fields)
{
	const auto found = std::find(fields.begin(), fields.end(), scalar_field);
	if (found == fields.end()) {
		throw std::invalid_argument("Requested field is not a half-width field.");
	}
	return static_cast<std::size_t>(std::distance(fields.begin(), found));
}

bool isVerticalHeightField(VehicleSectionScalarField scalar_field)
{
	return std::find(
		kVerticalHeightFields.begin(), kVerticalHeightFields.end(), scalar_field) !=
		kVerticalHeightFields.end();
}

} // namespace

VehicleSemanticSectionFieldEvaluationService::
VehicleSemanticSectionFieldEvaluationService(
	const VehicleSemanticSectionField &section_field)
{
	if (section_field.stations().size() < 2u) {
		throw std::invalid_argument(
			"Semantic section evaluation requires at least two stations.");
	}
	rear_source_x_ = section_field.rearSourceX();
	front_source_x_ = section_field.frontSourceX();
	std::vector<double> coordinates;
	coordinates.reserve(section_field.stations().size());
	for (const VehicleSemanticSectionStation &station : section_field.stations()) {
		coordinates.push_back(station.sourceX());
	}

	VehicleSemanticSectionInterpolationService interpolation_service;
	vertical_height_interpolations_.reserve(kVerticalHeightFields.size());
	std::vector<double> underbody_heights;
	underbody_heights.reserve(section_field.stations().size());
	for (const VehicleSemanticSectionStation &station : section_field.stations()) {
		underbody_heights.push_back(
			scalarValue(station, VehicleSectionScalarField::UnderbodyHeight));
	}
	vertical_height_interpolations_.push_back(
		interpolation_service.createInterpolation(coordinates, underbody_heights));
	for (std::size_t upper_index = 1u;
	     upper_index < kVerticalHeightFields.size(); ++upper_index) {
		std::vector<double> logarithmic_gaps;
		logarithmic_gaps.reserve(section_field.stations().size());
		for (const VehicleSemanticSectionStation &station : section_field.stations()) {
			const double lower_height = scalarValue(
				station, kVerticalHeightFields[upper_index - 1u]);
			const double upper_height = scalarValue(
				station, kVerticalHeightFields[upper_index]);
			logarithmic_gaps.push_back(
				std::log(std::max(upper_height - lower_height, 1.0e-7)));
		}
		vertical_height_interpolations_.push_back(
			interpolation_service.createInterpolation(coordinates, logarithmic_gaps));
	}

	half_width_interpolations_.reserve(kHalfWidthFields.size());
	for (VehicleSectionScalarField scalar_field : kHalfWidthFields) {
		std::vector<double> values;
		values.reserve(section_field.stations().size());
		for (const VehicleSemanticSectionStation &station : section_field.stations()) {
			values.push_back(scalarValue(station, scalar_field));
		}
		half_width_interpolations_.push_back(
			interpolation_service.createInterpolation(coordinates, values));
	}
}

double VehicleSemanticSectionFieldEvaluationService::evaluate(
	VehicleSectionScalarField scalar_field,
	double source_x) const
{
	return isVerticalHeightField(scalar_field)
		? evaluateVerticalHeight(scalar_field, source_x)
		: evaluateHalfWidth(scalar_field, source_x);
}

double VehicleSemanticSectionFieldEvaluationService::evaluateVerticalHeight(
	VehicleSectionScalarField scalar_field,
	double source_x) const
{
	const std::size_t height_index = fieldIndex(scalar_field, kVerticalHeightFields);
	double height = vertical_height_interpolations_.front().evaluateAt(source_x);
	for (std::size_t gap_index = 1u; gap_index <= height_index; ++gap_index) {
		height += std::exp(
			vertical_height_interpolations_[gap_index].evaluateAt(source_x));
	}
	return height;
}

double VehicleSemanticSectionFieldEvaluationService::evaluateHalfWidth(
	VehicleSectionScalarField scalar_field,
	double source_x) const
{
	return half_width_interpolations_[fieldIndex(scalar_field, kHalfWidthFields)]
		.evaluateAt(source_x);
}

std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount>
VehicleSemanticSectionFieldEvaluationService::evaluateLandmarks(
	double source_x) const
{
	return {{
		{VehicleSectionLandmarkRole::Underbody,
		 evaluate(VehicleSectionScalarField::UnderbodyHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::UnderbodyHeight, source_x)},
		{VehicleSectionLandmarkRole::Rocker,
		 evaluate(VehicleSectionScalarField::RockerHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::RockerHeight, source_x)},
		{VehicleSectionLandmarkRole::LowerBody,
		 evaluate(VehicleSectionScalarField::LowerBodyHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::LowerBodyHeight, source_x)},
		{VehicleSectionLandmarkRole::Shoulder,
		 evaluate(VehicleSectionScalarField::ShoulderHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::ShoulderHeight, source_x)},
		{VehicleSectionLandmarkRole::Belt,
		 evaluate(VehicleSectionScalarField::BeltHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::BeltHeight, source_x)},
		{VehicleSectionLandmarkRole::GlassShoulder,
		 evaluate(VehicleSectionScalarField::GlassShoulderHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::GlassShoulderHeight, source_x)},
		{VehicleSectionLandmarkRole::RoofRail,
		 evaluate(VehicleSectionScalarField::RoofRailHalfWidth, source_x),
		 evaluate(VehicleSectionScalarField::RoofRailHeight, source_x)},
		{VehicleSectionLandmarkRole::RoofCrown,
		 0.0,
		 evaluate(VehicleSectionScalarField::RoofCrownHeight, source_x)}
	}};
}

double VehicleSemanticSectionFieldEvaluationService::halfWidthAt(
	double source_x,
	double source_height) const
{
	const auto landmarks = evaluateLandmarks(source_x);
	std::array<double, kVehicleSectionLandmarkCount> heights;
	std::array<double, kVehicleSectionLandmarkCount> widths;
	double previous_height = -1.0e100;
	for (std::size_t index = 0u; index < landmarks.size(); ++index) {
		heights[index] = std::max(
			landmarks[index].height() + static_cast<double>(index) * 1.0e-8,
			previous_height);
		widths[index] = landmarks[index].halfWidth();
		previous_height = heights[index];
	}
	if (source_height < heights.front() || source_height > heights.back()) return 0.0;
	for (std::size_t index = 0u; index + 1u < heights.size(); ++index) {
		if (source_height > heights[index + 1u]) continue;
		const double interval = heights[index + 1u] - heights[index];
		if (!(interval > 0.0)) return widths[index];
		const double parameter = (source_height - heights[index]) / interval;
		return widths[index] + (widths[index + 1u] - widths[index]) * parameter;
	}
	return widths.back();
}
