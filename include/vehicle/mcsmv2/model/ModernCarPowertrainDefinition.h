#pragma once

#include <string>
#include <utility>

class ModernCarPowertrainDefinition
{
public:
	ModernCarPowertrainDefinition(
		std::string architecture,
		std::string drive_layout,
		bool has_battery_pack,
		double battery_thickness,
		double engine_envelope_length,
		double engine_envelope_width,
		double engine_envelope_height,
		int exhaust_count)
		: architecture_(std::move(architecture)),
		  drive_layout_(std::move(drive_layout)),
		  has_battery_pack_(has_battery_pack),
		  battery_thickness_(battery_thickness),
		  engine_envelope_length_(engine_envelope_length),
		  engine_envelope_width_(engine_envelope_width),
		  engine_envelope_height_(engine_envelope_height),
		  exhaust_count_(exhaust_count)
	{
	}

	const std::string &architecture() const { return architecture_; }
	const std::string &driveLayout() const { return drive_layout_; }
	bool hasBatteryPack() const { return has_battery_pack_; }
	double batteryThickness() const { return battery_thickness_; }
	double engineEnvelopeLength() const { return engine_envelope_length_; }
	double engineEnvelopeWidth() const { return engine_envelope_width_; }
	double engineEnvelopeHeight() const { return engine_envelope_height_; }
	int exhaustCount() const { return exhaust_count_; }

private:
	std::string architecture_;
	std::string drive_layout_;
	bool has_battery_pack_ = false;
	double battery_thickness_ = 0.0;
	double engine_envelope_length_ = 0.0;
	double engine_envelope_width_ = 0.0;
	double engine_envelope_height_ = 0.0;
	int exhaust_count_ = 0;
};
