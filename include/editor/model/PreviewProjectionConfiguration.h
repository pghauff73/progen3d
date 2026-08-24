#pragma once

class PreviewProjectionConfiguration
{
public:
	static constexpr float minimum_vertical_field_of_view_degrees = 15.0f;
	static constexpr float default_vertical_field_of_view_degrees = 43.0f;
	static constexpr float maximum_vertical_field_of_view_degrees = 100.0f;

	float verticalFieldOfViewDegrees() const;
	float verticalFieldOfViewRadians() const;

	void setVerticalFieldOfViewDegrees(float requested_degrees);
	void reset();

private:
	float vertical_field_of_view_degrees_ =
		default_vertical_field_of_view_degrees;
};
