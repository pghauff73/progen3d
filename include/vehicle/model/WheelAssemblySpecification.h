#pragma once

#include "geometry/model/GeometryDetailLevel.h"

#include <string>
#include <utility>

class WheelAssemblySpecification
{
public:
	WheelAssemblySpecification(
		std::string object_identifier,
		float tire_radius,
		float tire_section_width,
		float rim_radius,
		float rim_width,
		float hub_radius,
		float brake_radius,
		int spoke_count,
		std::string tire_material,
		std::string rim_material,
		std::string brake_material,
		GeometryDetailLevel detail_level)
		: object_identifier_(std::move(object_identifier)),
		  tire_radius_(tire_radius),
		  tire_section_width_(tire_section_width),
		  rim_radius_(rim_radius),
		  rim_width_(rim_width),
		  hub_radius_(hub_radius),
		  brake_radius_(brake_radius),
		  spoke_count_(spoke_count),
		  tire_material_(std::move(tire_material)),
		  rim_material_(std::move(rim_material)),
		  brake_material_(std::move(brake_material)),
		  detail_level_(detail_level)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	float tireRadius() const { return tire_radius_; }
	float tireSectionWidth() const { return tire_section_width_; }
	float rimRadius() const { return rim_radius_; }
	float rimWidth() const { return rim_width_; }
	float hubRadius() const { return hub_radius_; }
	float brakeRadius() const { return brake_radius_; }
	int spokeCount() const { return spoke_count_; }
	const std::string &tireMaterial() const { return tire_material_; }
	const std::string &rimMaterial() const { return rim_material_; }
	const std::string &brakeMaterial() const { return brake_material_; }
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::string object_identifier_;
	float tire_radius_ = 0.0f;
	float tire_section_width_ = 0.0f;
	float rim_radius_ = 0.0f;
	float rim_width_ = 0.0f;
	float hub_radius_ = 0.0f;
	float brake_radius_ = 0.0f;
	int spoke_count_ = 0;
	std::string tire_material_;
	std::string rim_material_;
	std::string brake_material_;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::Component;
};
