#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "geometry/model/GeometryDetailRange.h"
#include "vehicle/model/VehicleAssemblyGeometry.h"
#include "vehicle/model/VehicleLoftPanelSection.h"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VehiclePrimitivePartBuildResult
{
public:
	static VehiclePrimitivePartBuildResult succeeded(VehicleAssemblyPart part)
	{
		return VehiclePrimitivePartBuildResult(std::move(part), {});
	}

	static VehiclePrimitivePartBuildResult failed(std::string diagnostic)
	{
		return VehiclePrimitivePartBuildResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return part_.has_value(); }
	const std::optional<VehicleAssemblyPart> &part() const { return part_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VehiclePrimitivePartBuildResult(
		std::optional<VehicleAssemblyPart> part,
		std::string diagnostic)
		: part_(std::move(part)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VehicleAssemblyPart> part_;
	std::string diagnostic_;
};

class VehiclePrimitivePartFactory
{
public:
	explicit VehiclePrimitivePartFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehiclePrimitivePartBuildResult buildCenteredRoundedBox(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		glm::vec3 dimensions,
		float corner_radius,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range,
		GeometryDetailLevel detail_level) const;

	VehiclePrimitivePartBuildResult buildLoftedSolid(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		std::vector<VehicleLoftPanelSection> sections,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range,
		GeometryDetailLevel detail_level) const;

	VehiclePrimitivePartBuildResult buildSweepDisk(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		std::vector<glm::vec3> path,
		float radius,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range,
		GeometryDetailLevel detail_level) const;

	VehiclePrimitivePartBuildResult buildRevolvedSolid(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		std::vector<glm::vec2> radial_profile,
		int angular_segments,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range,
		GeometryDetailLevel detail_level) const;

	VehiclePrimitivePartBuildResult buildInstanceArray(
		std::string part_identifier,
		std::string semantic_role,
		std::string material_identifier,
		const VehicleAssemblyPart &source_part,
		std::vector<glm::mat4> instance_transforms,
		glm::mat4 local_transform,
		GeometryDetailRange detail_range,
		GeometryDetailLevel detail_level) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
