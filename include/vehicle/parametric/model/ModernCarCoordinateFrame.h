#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class McsM1Coordinate
{
public:
	explicit McsM1Coordinate(glm::dvec3 value = glm::dvec3(0.0)) : value_(value) {}

	const glm::dvec3 &value() const { return value_; }

private:
	glm::dvec3 value_{0.0};
};

class McpVehicleCoordinate
{
public:
	explicit McpVehicleCoordinate(glm::dvec3 value = glm::dvec3(0.0)) : value_(value) {}

	const glm::dvec3 &value() const { return value_; }

private:
	glm::dvec3 value_{0.0};
};

class ModernCarCoordinateFrame
{
public:
	explicit ModernCarCoordinateFrame(
		std::string identifier = "MCP_OMv1.VehicleCoordinateFrame")
		: identifier_(std::move(identifier))
	{
	}

	const std::string &identifier() const { return identifier_; }

	McpVehicleCoordinate convertSourceToMcp(
		const McsM1Coordinate &source,
		double source_package_center_x) const
	{
		const glm::dvec3 &point = source.value();
		return McpVehicleCoordinate(
			glm::dvec3(point.y, point.z, point.x - source_package_center_x));
	}

	McsM1Coordinate convertMcpToSource(
		const McpVehicleCoordinate &point,
		double source_package_center_x) const
	{
		const glm::dvec3 &value = point.value();
		return McsM1Coordinate(
			glm::dvec3(value.z + source_package_center_x, value.x, value.y));
	}

private:
	std::string identifier_;
};
