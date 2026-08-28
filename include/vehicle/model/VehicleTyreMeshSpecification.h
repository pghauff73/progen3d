#pragma once

#include <cstddef>

class VehicleTyreMeshSpecification
{
public:
	VehicleTyreMeshSpecification(
		double wheel_radius_metres,
		double wheel_width_metres,
		std::size_t circumferential_section_count = 40u,
		std::size_t cross_section_count = 14u)
		: wheel_radius_metres_(wheel_radius_metres),
		  wheel_width_metres_(wheel_width_metres),
		  circumferential_section_count_(circumferential_section_count),
		  cross_section_count_(cross_section_count)
	{
	}

	double wheelRadiusMetres() const { return wheel_radius_metres_; }
	double wheelWidthMetres() const { return wheel_width_metres_; }
	std::size_t circumferentialSectionCount() const
	{
		return circumferential_section_count_;
	}
	std::size_t crossSectionCount() const { return cross_section_count_; }

private:
	double wheel_radius_metres_ = 0.0;
	double wheel_width_metres_ = 0.0;
	std::size_t circumferential_section_count_ = 0u;
	std::size_t cross_section_count_ = 0u;
};
