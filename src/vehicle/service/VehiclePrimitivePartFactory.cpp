#include "vehicle/service/VehiclePrimitivePartFactory.h"

#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/model/RoundedBoxSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/RevolveMeshGenerator.h"
#include "geometry/service/RevolveSpecificationValidator.h"
#include "geometry/service/RoundedBoxShapeSpecificationFactory.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <sstream>
#include <utility>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

bool finite(const glm::mat4 &value)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(value[column][row])) return false;
		}
	}
	return true;
}

VehiclePrimitivePartBuildResult invalid_part_request(
	const std::string &part_identifier)
{
	return VehiclePrimitivePartBuildResult::failed(
		"Vehicle primitive part '" + part_identifier +
		"' requires non-empty identifiers, finite geometry, and a valid detail range.");
}

bool valid_part_request(
	const std::string &part_identifier,
	const std::string &semantic_role,
	const std::string &material_identifier,
	const glm::mat4 &local_transform,
	const GeometryDetailRange &detail_range)
{
	return !part_identifier.empty() && !semantic_role.empty() &&
	       !material_identifier.empty() && finite(local_transform) &&
	       detail_range.isValid();
}

} // namespace

VehiclePrimitivePartBuildResult
VehiclePrimitivePartFactory::buildCenteredRoundedBox(
	std::string part_identifier,
	std::string semantic_role,
	std::string material_identifier,
	glm::vec3 dimensions,
	float corner_radius,
	glm::mat4 local_transform,
	GeometryDetailRange detail_range,
	GeometryDetailLevel detail_level) const
{
	if (!valid_part_request(
			part_identifier, semantic_role, material_identifier,
			local_transform, detail_range) ||
	    !finite(dimensions) || dimensions.x <= 0.0f || dimensions.y <= 0.0f ||
	    dimensions.z <= 0.0f || !std::isfinite(corner_radius) ||
	    corner_radius <= 0.0f) {
		return invalid_part_request(part_identifier);
	}

	std::string diagnostic;
	auto shape = RoundedBoxShapeSpecificationFactory(complexity_limits_).create(
		RoundedBoxSpecification(
			dimensions.x, dimensions.y, dimensions.z, corner_radius, 4),
		detail_level, &diagnostic);
	if (!shape) return VehiclePrimitivePartBuildResult::failed(diagnostic);
	GeometryBuildResult built =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(built.firstDiagnostic());
	}
	local_transform = local_transform * glm::translate(
		glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -dimensions.z * 0.5f));
	return VehiclePrimitivePartBuildResult::succeeded(VehicleAssemblyPart(
		std::move(part_identifier), std::move(semantic_role),
		std::move(material_identifier), std::move(shape), built.generatedMesh(),
		local_transform, detail_range));
}

VehiclePrimitivePartBuildResult
VehiclePrimitivePartFactory::buildLoftedSolid(
	std::string part_identifier,
	std::string semantic_role,
	std::string material_identifier,
	std::vector<VehicleLoftPanelSection> sections,
	glm::mat4 local_transform,
	GeometryDetailRange detail_range,
	GeometryDetailLevel detail_level) const
{
	if (!valid_part_request(
			part_identifier, semantic_role, material_identifier,
			local_transform, detail_range) ||
	    sections.size() < 2u) {
		return invalid_part_request(part_identifier);
	}

	LoftShapeSpecificationCandidate candidate;
	candidate.detail_level = detail_level;
	for (VehicleLoftPanelSection &section : sections) {
		LoftSectionCandidate loft_section;
		loft_section.axial_position = section.stationZ();
		loft_section.profile.outer_loop = section.profile();
		candidate.sections.push_back(std::move(loft_section));
	}
	std::string diagnostic;
	auto shape = LoftSpecificationValidator(complexity_limits_).validateLoft(
		std::move(candidate), &diagnostic);
	if (!shape) return VehiclePrimitivePartBuildResult::failed(diagnostic);
	GeometryBuildResult built = LoftMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(built.firstDiagnostic());
	}
	return VehiclePrimitivePartBuildResult::succeeded(VehicleAssemblyPart(
		std::move(part_identifier), std::move(semantic_role),
		std::move(material_identifier), std::move(shape), built.generatedMesh(),
		local_transform, detail_range));
}

VehiclePrimitivePartBuildResult VehiclePrimitivePartFactory::buildSweepDisk(
	std::string part_identifier,
	std::string semantic_role,
	std::string material_identifier,
	std::vector<glm::vec3> path,
	float radius,
	glm::mat4 local_transform,
	GeometryDetailRange detail_range,
	GeometryDetailLevel detail_level) const
{
	if (!valid_part_request(
			part_identifier, semantic_role, material_identifier,
			local_transform, detail_range) ||
	    path.size() < 2u || !std::isfinite(radius) || radius <= 0.0f) {
		return invalid_part_request(part_identifier);
	}
	SweepDiskShapeSpecificationCandidate candidate;
	candidate.path_points = std::move(path);
	candidate.radius = radius;
	candidate.circumferential_segments = 12;
	candidate.detail_level = detail_level;
	std::string diagnostic;
	auto shape = SweepDiskSpecificationValidator(complexity_limits_).validate(
		std::move(candidate), &diagnostic);
	if (!shape) return VehiclePrimitivePartBuildResult::failed(diagnostic);
	GeometryBuildResult built = SweepDiskMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(built.firstDiagnostic());
	}
	return VehiclePrimitivePartBuildResult::succeeded(VehicleAssemblyPart(
		std::move(part_identifier), std::move(semantic_role),
		std::move(material_identifier), std::move(shape), built.generatedMesh(),
		local_transform, detail_range));
}

VehiclePrimitivePartBuildResult
VehiclePrimitivePartFactory::buildRevolvedSolid(
	std::string part_identifier,
	std::string semantic_role,
	std::string material_identifier,
	std::vector<glm::vec2> radial_profile,
	int angular_segments,
	glm::mat4 local_transform,
	GeometryDetailRange detail_range,
	GeometryDetailLevel detail_level) const
{
	if (!valid_part_request(
			part_identifier, semantic_role, material_identifier,
			local_transform, detail_range) ||
	    radial_profile.size() < 3u || angular_segments < 3) {
		return invalid_part_request(part_identifier);
	}
	RevolveShapeSpecificationCandidate candidate;
	candidate.radial_profile.outer_loop = std::move(radial_profile);
	candidate.angular_segments = angular_segments;
	candidate.detail_level = detail_level;
	std::string diagnostic;
	auto shape = RevolveSpecificationValidator(complexity_limits_).validate(
		std::move(candidate), &diagnostic);
	if (!shape) return VehiclePrimitivePartBuildResult::failed(diagnostic);
	GeometryBuildResult built = RevolveMeshGenerator(complexity_limits_).build(*shape);
	if (!built.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(built.firstDiagnostic());
	}
	return VehiclePrimitivePartBuildResult::succeeded(VehicleAssemblyPart(
		std::move(part_identifier), std::move(semantic_role),
		std::move(material_identifier), std::move(shape), built.generatedMesh(),
		local_transform, detail_range));
}

VehiclePrimitivePartBuildResult
VehiclePrimitivePartFactory::buildInstanceArray(
	std::string part_identifier,
	std::string semantic_role,
	std::string material_identifier,
	const VehicleAssemblyPart &source_part,
	std::vector<glm::mat4> instance_transforms,
	glm::mat4 local_transform,
	GeometryDetailRange detail_range,
	GeometryDetailLevel detail_level) const
{
	if (!valid_part_request(
			part_identifier, semantic_role, material_identifier,
			local_transform, detail_range) ||
	    !source_part.shape() || !source_part.generatedMesh().mesh() ||
	    instance_transforms.empty()) {
		return invalid_part_request(part_identifier);
	}
	InstanceArraySpecification array(std::move(instance_transforms));
	GeometryBuildResult centered_source = GeneratedMeshComposer(complexity_limits_).compose(
		{GeneratedMeshPlacement(
			"vehicle_array_source", source_part.generatedMesh(),
			source_part.localTransform())});
	if (!centered_source.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(
			centered_source.firstDiagnostic());
	}
	GeometryBuildResult built = InstanceArrayGeometryBuilder(complexity_limits_).build(
		centered_source.generatedMesh(), array);
	if (!built.succeeded()) {
		return VehiclePrimitivePartBuildResult::failed(built.firstDiagnostic());
	}
	std::ostringstream canonical;
	canonical << "VehicleInstanceArray:v1:source="
	          << source_part.shape()->key().canonicalValue() << ":count="
	          << array.transforms().size();
	auto shape = std::make_shared<const InstanceArrayShapeSpecification>(
		source_part.shape(), std::move(array),
		ShapeSpecificationKey(canonical.str()), canonical.str(), detail_level);
	return VehiclePrimitivePartBuildResult::succeeded(VehicleAssemblyPart(
		std::move(part_identifier), std::move(semantic_role),
		std::move(material_identifier), std::move(shape), built.generatedMesh(),
		local_transform, detail_range));
}
