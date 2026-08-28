#include "geometry/service/LoftMeshGenerator.h"

#include "geometry/model/LoftShapeSpecification.h"
#include "geometry/service/ProfileCapTriangulator.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <utility>

namespace {

glm::vec3 transform_point(
	const glm::vec2 &point,
	const LoftSectionSpecification &section)
{
	const glm::vec2 scaled = point * section.scale();
	const float angle = glm::radians(section.rotationDegrees());
	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	const glm::vec2 rotated(
		scaled.x * cosine - scaled.y * sine,
		scaled.x * sine + scaled.y * cosine);
	const glm::vec2 positioned = rotated + section.center();
	return glm::vec3(positioned.x, positioned.y, section.axialPosition());
}

class LoftMeshConstruction
{
public:
	LoftMeshConstruction(
		const LoftShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		std::vector<ProfileTriangle2D> front_triangles;
		std::vector<ProfileTriangle2D> back_triangles;
		std::string diagnostic;
		if (specification_.capPolicy().closesFront() &&
		    !ProfileCapTriangulator().triangulate(
			    specification_.sections().front().profile(),
			    &front_triangles, &diagnostic)) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangulationFailure, diagnostic);
		}
		if (specification_.capPolicy().closesBack() &&
		    !ProfileCapTriangulator().triangulate(
			    specification_.sections().back().profile(),
			    &back_triangles, &diagnostic)) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangulationFailure, diagnostic);
		}

		const std::size_t side_triangle_count =
			specification_.sections().front().profile().pointCount() * 2u *
			(specification_.sections().size() - 1u);
		const std::size_t triangle_count =
			side_triangle_count + front_triangles.size() + back_triangles.size();
		if (triangle_count > complexity_limits_.maximumGeneratedTriangles() ||
		    triangle_count > complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"Loft exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);

		addLoopSurfaces(false, MeshSurfaceRole::ProfileOuterSide, 0u);
		for (std::size_t hole_index = 0;
		     hole_index < specification_.sections().front().profile().innerLoops().size();
		     ++hole_index) {
			addLoopSurfaces(true, MeshSurfaceRole::ProfileInnerSide, hole_index);
		}
		for (const ProfileTriangle2D &triangle : front_triangles) {
			addCapTriangle(
				triangle, specification_.sections().front(),
				MeshSurfaceRole::ProfileFrontCap, true);
		}
		for (const ProfileTriangle2D &triangle : back_triangles) {
			addCapTriangle(
				triangle, specification_.sections().back(),
				MeshSurfaceRole::ProfileBackCap, false);
		}

		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	int addVertex(const glm::vec3 &position,
	              const glm::vec3 &normal,
	              const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->normals.push_back(normal);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1u);
	}

	void addTriangle(const glm::vec3 &first,
	                 const glm::vec3 &second,
	                 const glm::vec3 &third,
	                 const glm::vec3 &first_texcoord,
	                 const glm::vec3 &second_texcoord,
	                 const glm::vec3 &third_texcoord,
	                 MeshSurfaceRole role,
	                 std::size_t boundary_index)
	{
		const glm::vec3 cross = glm::cross(second - first, third - first);
		if (glm::dot(cross, cross) <= 1.0e-16f) return;
		const glm::vec3 normal = glm::normalize(cross);
		const int first_index = addVertex(first, normal, first_texcoord);
		const int second_index = addVertex(second, normal, second_texcoord);
		const int third_index = addVertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, boundary_index);
	}

	const ProfileLoop2D &loopFor(
		const LoftSectionSpecification &section,
		bool inner,
		std::size_t boundary_index) const
	{
		return inner
			? section.profile().innerLoops()[boundary_index]
			: section.profile().outerLoop();
	}

	void addLoopSurfaces(
		bool inner,
		MeshSurfaceRole role,
		std::size_t boundary_index)
	{
		const ProfileLoop2D &reference_loop = loopFor(
			specification_.sections().front(), inner, boundary_index);
		for (std::size_t section_index = 0;
		     section_index + 1u < specification_.sections().size();
		     ++section_index) {
			const LoftSectionSpecification &first_section =
				specification_.sections()[section_index];
			const LoftSectionSpecification &second_section =
				specification_.sections()[section_index + 1u];
			const ProfileLoop2D &first_loop =
				loopFor(first_section, inner, boundary_index);
			const ProfileLoop2D &second_loop =
				loopFor(second_section, inner, boundary_index);
			for (std::size_t point_index = 0;
			     point_index < reference_loop.points().size();
			     ++point_index) {
				const std::size_t next_index =
					(point_index + 1u) % reference_loop.points().size();
				const glm::vec3 first =
					transform_point(first_loop.points()[point_index], first_section);
				const glm::vec3 second =
					transform_point(first_loop.points()[next_index], first_section);
				const glm::vec3 third =
					transform_point(second_loop.points()[next_index], second_section);
				const glm::vec3 fourth =
					transform_point(second_loop.points()[point_index], second_section);
				const float u0 = static_cast<float>(point_index) /
				                 static_cast<float>(reference_loop.points().size());
				const float u1 = static_cast<float>(point_index + 1u) /
				                 static_cast<float>(reference_loop.points().size());
				const float v0 = static_cast<float>(section_index) /
				                 static_cast<float>(specification_.sections().size() - 1u);
				const float v1 = static_cast<float>(section_index + 1u) /
				                 static_cast<float>(specification_.sections().size() - 1u);
				addTriangle(first, second, third,
				            glm::vec3(u0, v0, 0.0f),
				            glm::vec3(u1, v0, 0.0f),
				            glm::vec3(u1, v1, 0.0f), role, boundary_index);
				addTriangle(first, third, fourth,
				            glm::vec3(u0, v0, 0.0f),
				            glm::vec3(u1, v1, 0.0f),
				            glm::vec3(u0, v1, 0.0f), role, boundary_index);
			}
		}
	}

	void addCapTriangle(
		const ProfileTriangle2D &triangle,
		const LoftSectionSpecification &section,
		MeshSurfaceRole role,
		bool reverse)
	{
		const glm::vec3 first = transform_point(triangle.first, section);
		const glm::vec3 second = transform_point(triangle.second, section);
		const glm::vec3 third = transform_point(triangle.third, section);
		if (reverse) {
			addTriangle(first, third, second,
			            glm::vec3(triangle.first, 0.0f),
			            glm::vec3(triangle.third, 0.0f),
			            glm::vec3(triangle.second, 0.0f), role, 0u);
		}
		else {
			addTriangle(first, second, third,
			            glm::vec3(triangle.first, 0.0f),
			            glm::vec3(triangle.second, 0.0f),
			            glm::vec3(triangle.third, 0.0f), role, 0u);
		}
	}

	const LoftShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

}

GeometryBuildResult LoftMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *loft = dynamic_cast<const LoftShapeSpecification *>(&specification);
	if (loft == nullptr ||
	    (specification.family() != ShapeFamily::Loft &&
	     specification.family() != ShapeFamily::SurfaceLoft &&
	     specification.family() != ShapeFamily::ShellLoft &&
	     specification.family() != ShapeFamily::FormedPanel)) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"LoftMeshGenerator requires a Loft, SurfaceLoft, ShellLoft, or FormedPanel specification.");
	}
	return LoftMeshConstruction(*loft, complexity_limits_).build();
}

GeneratedPrimitiveMesh LoftMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
