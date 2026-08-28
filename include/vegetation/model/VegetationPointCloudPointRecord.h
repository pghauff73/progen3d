#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/model/VegetationPointCloudSourceFormat.h"

#include <cstddef>
#include <cstdint>
#include <optional>

class VegetationPointCloudPointRecord
{
public:
	VegetationPointCloudPointRecord(
		std::size_t source_index,
		VegetationMeasuredPoint3d position_metres,
		std::optional<std::uint16_t> red,
		std::optional<std::uint16_t> green,
		std::optional<std::uint16_t> blue,
		std::optional<std::uint16_t> intensity,
		std::optional<std::uint8_t> classification,
		std::optional<std::uint8_t> return_number,
		VegetationPointCloudOrganClass organ_class)
		: source_index_(source_index),
		  position_metres_(position_metres),
		  red_(red),
		  green_(green),
		  blue_(blue),
		  intensity_(intensity),
		  classification_(classification),
		  return_number_(return_number),
		  organ_class_(organ_class)
	{
	}

	std::size_t sourceIndex() const { return source_index_; }
	const VegetationMeasuredPoint3d &positionMetres() const
	{
		return position_metres_;
	}
	const std::optional<std::uint16_t> &red() const { return red_; }
	const std::optional<std::uint16_t> &green() const { return green_; }
	const std::optional<std::uint16_t> &blue() const { return blue_; }
	const std::optional<std::uint16_t> &intensity() const { return intensity_; }
	const std::optional<std::uint8_t> &classification() const
	{
		return classification_;
	}
	const std::optional<std::uint8_t> &returnNumber() const
	{
		return return_number_;
	}
	VegetationPointCloudOrganClass organClass() const { return organ_class_; }

private:
	std::size_t source_index_ = 0u;
	VegetationMeasuredPoint3d position_metres_;
	std::optional<std::uint16_t> red_;
	std::optional<std::uint16_t> green_;
	std::optional<std::uint16_t> blue_;
	std::optional<std::uint16_t> intensity_;
	std::optional<std::uint8_t> classification_;
	std::optional<std::uint8_t> return_number_;
	VegetationPointCloudOrganClass organ_class_ =
		VegetationPointCloudOrganClass::Unknown;
};
