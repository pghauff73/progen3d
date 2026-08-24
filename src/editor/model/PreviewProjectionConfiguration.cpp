#include "editor/model/PreviewProjectionConfiguration.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

}

float PreviewProjectionConfiguration::verticalFieldOfViewDegrees() const
{
	return vertical_field_of_view_degrees_;
}

float PreviewProjectionConfiguration::verticalFieldOfViewRadians() const
{
	return vertical_field_of_view_degrees_ * kPi / 180.0f;
}

void PreviewProjectionConfiguration::setVerticalFieldOfViewDegrees(
	float requested_degrees)
{
	if (!std::isfinite(requested_degrees)) {
		requested_degrees = default_vertical_field_of_view_degrees;
	}
	vertical_field_of_view_degrees_ = std::clamp(
		requested_degrees,
		minimum_vertical_field_of_view_degrees,
		maximum_vertical_field_of_view_degrees);
}

void PreviewProjectionConfiguration::reset()
{
	vertical_field_of_view_degrees_ = default_vertical_field_of_view_degrees;
}
