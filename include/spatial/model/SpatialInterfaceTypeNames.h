#pragma once

#include "spatial/model/SpatialInterfaceType.h"

#include <string_view>

struct SpatialInterfaceTypeName {
	SpatialInterfaceType type;
	std::string_view name;
};

inline constexpr SpatialInterfaceTypeName spatialInterfaceTypeNames[] = {
	{SpatialInterfaceType::Support, "Support"},
	{SpatialInterfaceType::Bearing, "Bearing"},
	{SpatialInterfaceType::Mate, "Mate"},
	{SpatialInterfaceType::Seat, "Seat"},
	{SpatialInterfaceType::Insert, "Insert"},
	{SpatialInterfaceType::Socket, "Socket"},
	{SpatialInterfaceType::Shaft, "Shaft"},
	{SpatialInterfaceType::Seal, "Seal"},
	{SpatialInterfaceType::Fastener, "Fastener"},
	{SpatialInterfaceType::Anchor, "Anchor"},
	{SpatialInterfaceType::Hinge, "Hinge"},
	{SpatialInterfaceType::Slide, "Slide"},
	{SpatialInterfaceType::PipePort, "PipePort"},
	{SpatialInterfaceType::DuctPort, "DuctPort"},
	{SpatialInterfaceType::ElectricalPort, "ElectricalPort"},
	{SpatialInterfaceType::DataPort, "DataPort"},
	{SpatialInterfaceType::ControlPort, "ControlPort"},
	{SpatialInterfaceType::DrainPort, "DrainPort"},
	{SpatialInterfaceType::ThermalInterface, "ThermalInterface"},
	{SpatialInterfaceType::InspectionInterface, "InspectionInterface"},
};
