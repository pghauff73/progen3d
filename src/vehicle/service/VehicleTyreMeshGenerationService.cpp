#include "vehicle/service/VehicleTyreMeshGenerationService.h"

#include "geometry/model/MeshSurfaceTag.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

} // namespace

GeneratedPrimitiveMesh
VehicleTyreMeshGenerationService::generateSourceFrameTyre(
	const VehicleTyreMeshSpecification &specification) const
{
	validateSpecification(specification);
	const std::size_t major_sections =
		specification.circumferentialSectionCount();
	const std::size_t minor_sections = specification.crossSectionCount();
	const double wheel_radius = specification.wheelRadiusMetres();
	const double rim_radius =
		std::max(wheel_radius * 0.69, wheel_radius - 0.105);
	const double radial_radius = 0.5 * (wheel_radius - rim_radius);
	const double major_radius = rim_radius + radial_radius;
	const double lateral_radius = 0.5 * specification.wheelWidthMetres();

	auto mesh = std::make_shared<Mesh>();
	mesh->vertices.reserve(major_sections * minor_sections);
	mesh->faces.reserve(major_sections * minor_sections * 2u);
	for (std::size_t major_index = 0u; major_index < major_sections;
	     ++major_index) {
		const double theta =
			2.0 * kPi * static_cast<double>(major_index) /
			static_cast<double>(major_sections);
		const double cosine_theta = std::cos(theta);
		const double sine_theta = std::sin(theta);
		for (std::size_t minor_index = 0u; minor_index < minor_sections;
		     ++minor_index) {
			const double phi =
				2.0 * kPi * static_cast<double>(minor_index) /
				static_cast<double>(minor_sections);
			const double radius =
				major_radius + radial_radius * std::cos(phi);
			mesh->vertices.emplace_back(
				static_cast<float>(radius * cosine_theta),
				static_cast<float>(lateral_radius * std::sin(phi)),
				static_cast<float>(radius * sine_theta));
		}
	}

	for (std::size_t major_index = 0u; major_index < major_sections;
	     ++major_index) {
		const std::size_t next_major_index = (major_index + 1u) % major_sections;
		for (std::size_t minor_index = 0u; minor_index < minor_sections;
		     ++minor_index) {
			const std::size_t next_minor_index =
				(minor_index + 1u) % minor_sections;
			const int first = static_cast<int>(
				major_index * minor_sections + minor_index);
			const int second = static_cast<int>(
				next_major_index * minor_sections + minor_index);
			const int third = static_cast<int>(
				next_major_index * minor_sections + next_minor_index);
			const int fourth = static_cast<int>(
				major_index * minor_sections + next_minor_index);
			mesh->faces.emplace_back(first, second, third);
			mesh->faces.emplace_back(first, third, fourth);
		}
	}
	mesh->calc_normals();

	return GeneratedPrimitiveMesh(
		std::move(mesh),
		std::vector<MeshSurfaceTag>(
			major_sections * minor_sections * 2u,
			MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

GeneratedPrimitiveMesh
VehicleTyreMeshGenerationService::generateTransformedTyre(
	const VehicleTyreMeshSpecification &specification,
	const glm::dmat4 &source_transform) const
{
	GeneratedPrimitiveMesh generated = generateSourceFrameTyre(specification);
	generated.mesh()->apply(glm::mat4(source_transform));
	generated.mesh()->calc_normals();
	return generated;
}

void VehicleTyreMeshGenerationService::validateSpecification(
	const VehicleTyreMeshSpecification &specification) const
{
	if (!std::isfinite(specification.wheelRadiusMetres()) ||
	    specification.wheelRadiusMetres() <= 0.0) {
		throw std::invalid_argument("Tyre wheel radius must be finite and positive.");
	}
	if (!std::isfinite(specification.wheelWidthMetres()) ||
	    specification.wheelWidthMetres() <= 0.0) {
		throw std::invalid_argument("Tyre wheel width must be finite and positive.");
	}
	if (specification.circumferentialSectionCount() < 3u) {
		throw std::invalid_argument(
			"Tyre circumferential section count must be at least three.");
	}
	if (specification.crossSectionCount() < 3u) {
		throw std::invalid_argument(
			"Tyre cross-section count must be at least three.");
	}
}
