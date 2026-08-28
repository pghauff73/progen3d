#pragma once

#include <glm/glm.hpp>

enum class PhotometricIntensityUnit
{
	Lumens,
	Candela,
	Lux
};

class PhotometricEmission
{
public:
	const glm::vec3 &linearRgb() const { return linear_rgb_; }
	void setLinearRgb(const glm::vec3 &linear_rgb) { linear_rgb_ = linear_rgb; }

	float colorTemperatureKelvin() const { return color_temperature_kelvin_; }
	void setColorTemperatureKelvin(float kelvin) { color_temperature_kelvin_ = kelvin; }

	bool usesColorTemperature() const { return use_color_temperature_; }
	void setUsesColorTemperature(bool use_color_temperature)
	{
		use_color_temperature_ = use_color_temperature;
	}

	float intensity() const { return intensity_; }
	void setIntensity(float intensity) { intensity_ = intensity; }

	PhotometricIntensityUnit intensityUnit() const { return intensity_unit_; }
	void setIntensityUnit(PhotometricIntensityUnit intensity_unit)
	{
		intensity_unit_ = intensity_unit;
	}

	float exposureCompensation() const { return exposure_compensation_; }
	void setExposureCompensation(float exposure_compensation)
	{
		exposure_compensation_ = exposure_compensation;
	}

private:
	glm::vec3 linear_rgb_{1.0f};
	float color_temperature_kelvin_ = 6500.0f;
	bool use_color_temperature_ = false;
	float intensity_ = 1000.0f;
	PhotometricIntensityUnit intensity_unit_ = PhotometricIntensityUnit::Lumens;
	float exposure_compensation_ = 0.0f;
};
