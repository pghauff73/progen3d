#include "vehicle/mcsmv2/service/VehicleGlassApertureConstructionService.h"

#include "vehicle/mcsmv2/service/VehicleSurfaceDomainEvaluationService.h"
#include "vehicle/mcsmv2/service/VehicleSurfaceProjectionService.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::vector<glm::dvec3> calculateVertexNormals(const Mesh &mesh)
{
	std::vector<glm::dvec3> normals(mesh.vertices.size(), glm::dvec3(0.0));
	for (const glm::ivec3 &face : mesh.faces) {
		const glm::dvec3 first(mesh.vertices[static_cast<std::size_t>(face.x)]);
		const glm::dvec3 second(mesh.vertices[static_cast<std::size_t>(face.y)]);
		const glm::dvec3 third(mesh.vertices[static_cast<std::size_t>(face.z)]);
		const glm::dvec3 unnormalized = glm::cross(second - first, third - first);
		const double squared_length = glm::dot(unnormalized, unnormalized);
		if (squared_length <= 1.0e-24) continue;
		const glm::dvec3 normal = unnormalized / std::sqrt(squared_length);
		normals[static_cast<std::size_t>(face.x)] += normal;
		normals[static_cast<std::size_t>(face.y)] += normal;
		normals[static_cast<std::size_t>(face.z)] += normal;
	}
	for (glm::dvec3 &normal : normals) {
		const double squared_length = glm::dot(normal, normal);
		normal = squared_length > 1.0e-24
			? normal / std::sqrt(squared_length)
			: glm::dvec3(0.0, 1.0, 0.0);
	}
	return normals;
}

double calculateSurfaceArea(const Mesh &mesh)
{
	double area = 0.0;
	for (const glm::ivec3 &face : mesh.faces) {
		const glm::dvec3 first(mesh.vertices[static_cast<std::size_t>(face.x)]);
		const glm::dvec3 second(mesh.vertices[static_cast<std::size_t>(face.y)]);
		const glm::dvec3 third(mesh.vertices[static_cast<std::size_t>(face.z)]);
		area += 0.5 * std::sqrt(glm::dot(
			glm::cross(second - first, third - first),
			glm::cross(second - first, third - first)));
	}
	return area;
}

glm::dvec3 calculateFaceNormal(const Mesh &mesh, std::size_t face_index)
{
	const glm::ivec3 &face = mesh.faces.at(face_index);
	const glm::dvec3 first(mesh.vertices.at(static_cast<std::size_t>(face.x)));
	const glm::dvec3 second(mesh.vertices.at(static_cast<std::size_t>(face.y)));
	const glm::dvec3 third(mesh.vertices.at(static_cast<std::size_t>(face.z)));
	const glm::dvec3 normal = glm::cross(second - first, third - first);
	const double squared_length = glm::dot(normal, normal);
	return squared_length > 1.0e-24
		? normal / std::sqrt(squared_length)
		: glm::dvec3(0.0, 1.0, 0.0);
}

GeneratedPrimitiveMesh extractSupportMesh(
	const Mesh &registered_mesh,
	const std::vector<glm::dvec2> &surface_coordinates,
	const VehicleSurfaceDomain &aperture_domain)
{
	VehicleSurfaceDomainEvaluationService domain_evaluation;
	auto support_mesh = std::make_shared<Mesh>();
	std::vector<int> remapped_vertex_indices(
		registered_mesh.vertices.size(), -1);
	for (const glm::ivec3 &face : registered_mesh.faces) {
		const glm::dvec2 face_coordinate =
			domain_evaluation.calculateFaceCentroidCoordinate(
				face, surface_coordinates);
		if (!domain_evaluation.contains(aperture_domain, face_coordinate)) continue;
		glm::ivec3 remapped_face(0);
		const int source_indices[3] = {face.x, face.y, face.z};
		for (int corner = 0; corner < 3; ++corner) {
			const int source_index = source_indices[corner];
			int &remapped_index = remapped_vertex_indices[
				static_cast<std::size_t>(source_index)];
			if (remapped_index < 0) {
				remapped_index = static_cast<int>(support_mesh->vertices.size());
				support_mesh->vertices.push_back(
					registered_mesh.vertices[static_cast<std::size_t>(source_index)]);
			}
			remapped_face[corner] = remapped_index;
		}
		support_mesh->faces.push_back(remapped_face);
	}
	support_mesh->calc_normals();
	support_mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(
		support_mesh,
		std::vector<MeshSurfaceTag>(
			support_mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

VehicleGlassAperture constructAperture(
	const RegisteredVehicleSemanticSurface &registered_surface,
	const VehicleSurfaceDomain &aperture_domain,
	const Mesh &accelerated_body_surface,
	double outward_offset)
{
	const GeneratedPrimitiveMesh support = extractSupportMesh(
		*registered_surface.registeredSemanticMesh().mesh(),
		registered_surface.vertexSurfaceCoordinates(),
		aperture_domain);
	auto glass_mesh = std::make_shared<Mesh>(*support.mesh());
	const std::vector<glm::dvec3> support_normals =
		calculateVertexNormals(*support.mesh());
	for (std::size_t vertex_index = 0u;
	     vertex_index < glass_mesh->vertices.size();
	     ++vertex_index) {
		glass_mesh->vertices[vertex_index] +=
			glm::vec3(support_normals[vertex_index] * outward_offset);
	}
	const double support_area = calculateSurfaceArea(*support.mesh());
	VehicleSurfaceProjectionService projection_service;
	constexpr double minimum_clearance = 0.001;
	for (std::size_t vertex_index = 0u;
	     vertex_index < glass_mesh->vertices.size();
	     ++vertex_index) {
		for (int correction = 0; correction < 4; ++correction) {
			const glm::dvec3 glass_point(glass_mesh->vertices[vertex_index]);
			const std::optional<VehicleSurfaceProjection> projection =
				projection_service.projectPoint(accelerated_body_surface, glass_point);
			if (!projection) break;
			const glm::dvec3 body_normal = calculateFaceNormal(
				accelerated_body_surface, projection->faceIndex());
			const double signed_separation = glm::dot(
				glass_point - projection->projectedPoint(),
				body_normal);
			if (signed_separation >= minimum_clearance) break;
			glass_mesh->vertices[vertex_index] += glm::vec3(
				body_normal *
				(minimum_clearance - signed_separation + 1.0e-5));
		}
	}
	glass_mesh->calc_normals();
	glass_mesh->buildCollisionAccel();
	const GeneratedPrimitiveMesh glass(
		glass_mesh,
		std::vector<MeshSurfaceTag>(
			glass_mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
	double minimum_signed_separation = std::numeric_limits<double>::infinity();
	double maximum_signed_separation = 0.0;
	for (std::size_t vertex_index = 0u;
	     vertex_index < glass_mesh->vertices.size();
	     ++vertex_index) {
		const glm::dvec3 glass_point(glass_mesh->vertices[vertex_index]);
		const std::optional<VehicleSurfaceProjection> projection =
			projection_service.projectPoint(accelerated_body_surface, glass_point);
		if (!projection) {
			minimum_signed_separation = -std::numeric_limits<double>::infinity();
			break;
		}
		const glm::dvec3 body_normal = calculateFaceNormal(
			accelerated_body_surface, projection->faceIndex());
		const double signed_separation = glm::dot(
			glass_point - projection->projectedPoint(),
			body_normal);
		minimum_signed_separation =
			std::min(minimum_signed_separation, signed_separation);
		maximum_signed_separation =
			std::max(maximum_signed_separation, signed_separation);
	}
	const bool passed = !support.mesh()->faces.empty() &&
		support.mesh()->faces.size() >= 3u &&
		std::isfinite(support_area) && support_area > 1.0e-5 &&
		std::isfinite(minimum_signed_separation) &&
		minimum_signed_separation > 0.0005 &&
		maximum_signed_separation < 0.050;
	return VehicleGlassAperture(
		aperture_domain.identifier(),
		aperture_domain,
		support,
		glass,
		outward_offset,
		support_area,
		minimum_signed_separation,
		maximum_signed_separation,
		passed);
}

} // namespace

std::vector<VehicleGlassAperture>
VehicleGlassApertureConstructionService::construct(
	const RegisteredVehicleSemanticSurface &registered_surface,
	const VehicleSurfaceDomainCatalog &domain_catalog,
	const Mesh &final_body_surface,
	double outward_offset) const
{
	if (!registered_surface.registeredSemanticMesh().mesh() ||
	    !std::isfinite(outward_offset) || outward_offset <= 0.0) {
		throw std::invalid_argument(
			"Glass aperture construction requires registered geometry and a positive offset.");
	}
	Mesh accelerated_body_surface = final_body_surface;
	accelerated_body_surface.buildCollisionAccel();
	std::vector<VehicleGlassAperture> apertures;
	apertures.reserve(domain_catalog.apertureDomains().size());
	for (const VehicleSurfaceDomain &aperture_domain :
	     domain_catalog.apertureDomains()) {
		apertures.push_back(constructAperture(
			registered_surface,
			aperture_domain,
			accelerated_body_surface,
			outward_offset));
	}
	return apertures;
}
