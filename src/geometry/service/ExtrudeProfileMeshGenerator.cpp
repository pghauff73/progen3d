#include "geometry/service/ExtrudeProfileMeshGenerator.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/service/ProfileCapTriangulator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <memory>
#include <utility>

namespace {

class ExtrudeProfileMeshConstruction
{
public:
	ExtrudeProfileMeshConstruction(
		const ExtrudeProfileShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		std::vector<ProfileTriangle2D> cap_triangles;
		if (specification_.capPolicy().closesFront() ||
		    specification_.capPolicy().closesBack()) {
			std::string diagnostic;
			if (!ProfileCapTriangulator().triangulate(
					specification_.profile(), &cap_triangles, &diagnostic)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::TriangulationFailure,
					std::move(diagnostic));
			}
		}

		const std::size_t side_triangle_count =
			specification_.profile().pointCount() * 2u;
		const std::size_t cap_count =
			(specification_.capPolicy().closesFront() ? 1u : 0u) +
			(specification_.capPolicy().closesBack() ? 1u : 0u);
		const std::size_t triangle_count =
			side_triangle_count + cap_triangles.size() * cap_count;
		if (triangle_count > complexity_limits_.maximumGeneratedTriangles() ||
		    triangle_count > complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"ExtrudeProfile exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);

		addLoopSides(
			specification_.profile().outerLoop(),
			MeshSurfaceRole::ProfileOuterSide,
			0u);
		for (std::size_t hole_index = 0;
		     hole_index < specification_.profile().innerLoops().size();
		     ++hole_index) {
			addLoopSides(
				specification_.profile().innerLoops()[hole_index],
				MeshSurfaceRole::ProfileInnerSide,
				hole_index);
		}

		if (specification_.capPolicy().closesFront()) {
			for (const ProfileTriangle2D &triangle : cap_triangles) {
				addCapTriangle(
					triangle,
					0.0f,
					glm::vec3(0.0f, 0.0f, -1.0f),
					MeshSurfaceRole::ProfileFrontCap,
					true);
			}
		}
		if (specification_.capPolicy().closesBack()) {
			for (const ProfileTriangle2D &triangle : cap_triangles) {
				addCapTriangle(
					triangle,
					specification_.depth(),
					glm::vec3(0.0f, 0.0f, 1.0f),
					MeshSurfaceRole::ProfileBackCap,
					false);
			}
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
	                 const glm::vec3 &normal,
	                 const glm::vec3 &first_texcoord,
	                 const glm::vec3 &second_texcoord,
	                 const glm::vec3 &third_texcoord,
	                 MeshSurfaceRole role,
	                 std::size_t boundary_index)
	{
		const int first_index = addVertex(first, normal, first_texcoord);
		const int second_index = addVertex(second, normal, second_texcoord);
		const int third_index = addVertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, boundary_index);
	}

	void addLoopSides(const ProfileLoop2D &loop,
	                  MeshSurfaceRole role,
	                  std::size_t boundary_index)
	{
		float perimeter = 0.0f;
		for (std::size_t point_index = 0;
		     point_index < loop.points().size();
		     ++point_index) {
			perimeter += glm::length(
				loop.points()[(point_index + 1u) % loop.points().size()] -
				loop.points()[point_index]);
		}

		float current_distance = 0.0f;
		for (std::size_t point_index = 0;
		     point_index < loop.points().size();
		     ++point_index) {
			const glm::vec2 &current = loop.points()[point_index];
			const glm::vec2 &next =
				loop.points()[(point_index + 1u) % loop.points().size()];
			const glm::vec2 edge = next - current;
			const float edge_length = glm::length(edge);
			const glm::vec3 normal = glm::normalize(
				glm::vec3(edge.y, -edge.x, 0.0f));
			const float current_u = current_distance / perimeter;
			const float next_u = (current_distance + edge_length) / perimeter;

			const glm::vec3 front_current(current.x, current.y, 0.0f);
			const glm::vec3 front_next(next.x, next.y, 0.0f);
			const glm::vec3 back_next(next.x, next.y, specification_.depth());
			const glm::vec3 back_current(
				current.x, current.y, specification_.depth());
			addTriangle(
				front_current,
				front_next,
				back_next,
				normal,
				glm::vec3(current_u, 0.0f, 0.0f),
				glm::vec3(next_u, 0.0f, 0.0f),
				glm::vec3(next_u, 1.0f, 0.0f),
				role,
				boundary_index);
			addTriangle(
				front_current,
				back_next,
				back_current,
				normal,
				glm::vec3(current_u, 0.0f, 0.0f),
				glm::vec3(next_u, 1.0f, 0.0f),
				glm::vec3(current_u, 1.0f, 0.0f),
				role,
				boundary_index);
			current_distance += edge_length;
		}
	}

	void addCapTriangle(const ProfileTriangle2D &triangle,
	                    float z,
	                    const glm::vec3 &normal,
	                    MeshSurfaceRole role,
	                    bool reverse)
	{
		const glm::vec3 first(triangle.first.x, triangle.first.y, z);
		const glm::vec3 second(triangle.second.x, triangle.second.y, z);
		const glm::vec3 third(triangle.third.x, triangle.third.y, z);
		if (reverse) {
			addTriangle(
				first,
				third,
				second,
				normal,
				glm::vec3(triangle.first, 0.0f),
				glm::vec3(triangle.third, 0.0f),
				glm::vec3(triangle.second, 0.0f),
				role,
				0u);
		}
		else {
			addTriangle(
				first,
				second,
				third,
				normal,
				glm::vec3(triangle.first, 0.0f),
				glm::vec3(triangle.second, 0.0f),
				glm::vec3(triangle.third, 0.0f),
				role,
				0u);
		}
	}

	const ExtrudeProfileShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

}

GeometryBuildResult ExtrudeProfileMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *extrude =
		dynamic_cast<const ExtrudeProfileShapeSpecification *>(&specification);
	if (extrude == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"ExtrudeProfileMeshGenerator requires an ExtrudeProfileShapeSpecification.");
	}
	if (!std::isfinite(extrude->depth())) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::NonFiniteGeometry,
			"ExtrudeProfile depth must be finite.");
	}
	if (extrude->depth() <= 0.0f) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::InvalidProfile,
			"ExtrudeProfile depth must be greater than zero.");
	}
	return ExtrudeProfileMeshConstruction(*extrude, complexity_limits_).build();
}

GeneratedPrimitiveMesh ExtrudeProfileMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
