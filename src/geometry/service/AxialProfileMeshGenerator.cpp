#include "geometry/service/AxialProfileMeshGenerator.h"

#include "geometry/model/AxialProfileShapeSpecification.h"
#include "geometry/service/PolygonContainmentAnalyzer.h"
#include "geometry/service/SimplePolygonTriangulator.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <vector>

namespace {

struct AxialRing
{
	float axial_position = 0.0f;
	std::vector<glm::vec2> profile_vertices;
};

AxialRing build_ring(const AxialProfileShapeSpecification &specification,
	                 const AxialProfileLevel &level)
{
	const AxialProfilePolygon &profile =
		specification.profiles()[level.profileIndex()];
	const float radians = glm::radians(level.rotationDegrees());
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	AxialRing ring;
	ring.axial_position = level.axialPosition();
	ring.profile_vertices.reserve(profile.vertices().size());
	for (const glm::vec2 &vertex : profile.vertices()) {
		const glm::vec2 scaled(vertex.x * level.scale().x,
		                       vertex.y * level.scale().y);
		ring.profile_vertices.emplace_back(
			level.center().x + cosine * scaled.x - sine * scaled.y,
			level.center().y + sine * scaled.x + cosine * scaled.y);
	}
	return ring;
}

glm::vec3 embed(const glm::vec2 &profile_vertex, float axial_position)
{
	return glm::vec3(profile_vertex.x, axial_position, profile_vertex.y);
}

void append_triangle(Mesh *mesh,
	                 std::vector<MeshSurfaceTag> *surface_tags,
	                 const glm::vec3 &first,
	                 const glm::vec3 &second,
	                 const glm::vec3 &third,
	                 MeshSurfaceRole surface_role,
	                 std::size_t boundary_index)
{
	const glm::vec3 cross = glm::cross(second - first, third - first);
	if (glm::dot(cross, cross) <= 1.0e-16f) return;
	const glm::vec3 normal = glm::normalize(cross);
	const int first_index = static_cast<int>(mesh->vertices.size());
	mesh->vertices.push_back(first);
	mesh->vertices.push_back(second);
	mesh->vertices.push_back(third);
	mesh->normals.push_back(normal);
	mesh->normals.push_back(normal);
	mesh->normals.push_back(normal);
	mesh->texcoords.push_back(first);
	mesh->texcoords.push_back(second);
	mesh->texcoords.push_back(third);
	mesh->faces.emplace_back(first_index, first_index + 1, first_index + 2);
	surface_tags->emplace_back(surface_role, boundary_index);
}

void append_side_strip(const AxialRing &lower,
	                   const AxialRing &upper,
	                   std::size_t transition_index,
	                   Mesh *mesh,
	                   std::vector<MeshSurfaceTag> *surface_tags)
{
	for (std::size_t vertex_index = 0;
	     vertex_index < lower.profile_vertices.size();
	     ++vertex_index) {
		const std::size_t next =
			(vertex_index + 1) % lower.profile_vertices.size();
		const glm::vec3 lower_current = embed(
			lower.profile_vertices[vertex_index], lower.axial_position);
		const glm::vec3 lower_next = embed(
			lower.profile_vertices[next], lower.axial_position);
		const glm::vec3 upper_current = embed(
			upper.profile_vertices[vertex_index], upper.axial_position);
		const glm::vec3 upper_next = embed(
			upper.profile_vertices[next], upper.axial_position);
		append_triangle(mesh, surface_tags,
		                lower_current, upper_current, upper_next,
		                MeshSurfaceRole::AxialSide, transition_index);
		append_triangle(mesh, surface_tags,
		                lower_current, upper_next, lower_next,
		                MeshSurfaceRole::AxialSide, transition_index);
	}
}

void append_step_strip(const AxialRing &outer,
	                   const AxialRing &inner,
	                   bool upward_normal,
	                   std::size_t transition_index,
	                   Mesh *mesh,
	                   std::vector<MeshSurfaceTag> *surface_tags)
{
	for (std::size_t vertex_index = 0;
	     vertex_index < outer.profile_vertices.size();
	     ++vertex_index) {
		const std::size_t next =
			(vertex_index + 1) % outer.profile_vertices.size();
		const glm::vec3 outer_current = embed(
			outer.profile_vertices[vertex_index], outer.axial_position);
		const glm::vec3 outer_next = embed(
			outer.profile_vertices[next], outer.axial_position);
		const glm::vec3 inner_current = embed(
			inner.profile_vertices[vertex_index], inner.axial_position);
		const glm::vec3 inner_next = embed(
			inner.profile_vertices[next], inner.axial_position);
		if (upward_normal) {
			append_triangle(mesh, surface_tags,
			                outer_current, inner_next, outer_next,
			                MeshSurfaceRole::AxialStep, transition_index);
			append_triangle(mesh, surface_tags,
			                outer_current, inner_current, inner_next,
			                MeshSurfaceRole::AxialStep, transition_index);
		} else {
			append_triangle(mesh, surface_tags,
			                outer_current, outer_next, inner_next,
			                MeshSurfaceRole::AxialStep, transition_index);
			append_triangle(mesh, surface_tags,
			                outer_current, inner_next, inner_current,
			                MeshSurfaceRole::AxialStep, transition_index);
		}
	}
}

bool append_cap(const AxialRing &ring,
	            bool upward_normal,
	            MeshSurfaceRole surface_role,
	            Mesh *mesh,
	            std::vector<MeshSurfaceTag> *surface_tags,
	            std::string *diagnostic)
{
	std::vector<glm::ivec3> triangles;
	if (!SimplePolygonTriangulator().triangulate(
		    ring.profile_vertices, &triangles, diagnostic)) {
		return false;
	}
	for (const glm::ivec3 &triangle : triangles) {
		const glm::vec3 first = embed(
			ring.profile_vertices[static_cast<std::size_t>(triangle.x)],
			ring.axial_position);
		const glm::vec3 second = embed(
			ring.profile_vertices[static_cast<std::size_t>(triangle.y)],
			ring.axial_position);
		const glm::vec3 third = embed(
			ring.profile_vertices[static_cast<std::size_t>(triangle.z)],
			ring.axial_position);
		if (upward_normal) {
			append_triangle(mesh, surface_tags, first, third, second,
			                surface_role, 0);
		} else {
			append_triangle(mesh, surface_tags, first, second, third,
			                surface_role, 0);
		}
	}
	return true;
}

}

GeneratedPrimitiveMesh AxialProfileMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *axial_profile =
		dynamic_cast<const AxialProfileShapeSpecification *>(&specification);
	if (axial_profile == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfileMeshGenerator received a non-AxialProfile specification.";
		}
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}

	std::vector<AxialRing> rings;
	rings.reserve(axial_profile->levels().size());
	for (const AxialProfileLevel &level : axial_profile->levels()) {
		rings.push_back(build_ring(*axial_profile, level));
	}

	auto mesh = std::make_shared<Mesh>();
	std::vector<MeshSurfaceTag> surface_tags;
	for (std::size_t level_index = 1;
	     level_index < axial_profile->levels().size();
	     ++level_index) {
		const AxialProfileLevel &level = axial_profile->levels()[level_index];
		if (level.transition() != AxialTransitionKind::Step) {
			append_side_strip(rings[level_index - 1], rings[level_index],
			                  level_index - 1, mesh.get(), &surface_tags);
			continue;
		}
		const auto relationship = PolygonContainmentAnalyzer().analyze(
			rings[level_index - 1].profile_vertices,
			rings[level_index].profile_vertices);
		if (relationship == PolygonContainmentRelationship::FirstContainsSecond) {
			append_step_strip(rings[level_index - 1], rings[level_index], true,
			                  level_index - 1, mesh.get(), &surface_tags);
		} else if (relationship == PolygonContainmentRelationship::SecondContainsFirst) {
			append_step_strip(rings[level_index], rings[level_index - 1], false,
			                  level_index - 1, mesh.get(), &surface_tags);
		} else {
			if (diagnostic != nullptr) {
				*diagnostic =
					"AxialProfile step profiles intersect and cannot be resolved by v1.";
			}
			return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
		}
	}

	if (axial_profile->capPolicy().capsBottom() &&
	    !append_cap(rings.front(), false, MeshSurfaceRole::AxialBottomCap,
	                mesh.get(), &surface_tags, diagnostic)) {
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	if (axial_profile->capPolicy().capsTop() &&
	    !append_cap(rings.back(), true, MeshSurfaceRole::AxialTopCap,
	                mesh.get(), &surface_tags, diagnostic)) {
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(mesh, std::move(surface_tags));
}
