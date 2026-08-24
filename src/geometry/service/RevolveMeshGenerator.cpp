#include "geometry/service/RevolveMeshGenerator.h"

#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/service/ProfileCapTriangulator.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <utility>

namespace {

constexpr float kAxisTolerance = 1.0e-6f;

glm::vec3 position_at(const glm::vec2 &profile_point, float angle)
{
	return glm::vec3(
		profile_point.x * std::cos(angle),
		profile_point.y,
		profile_point.x * std::sin(angle));
}

glm::vec3 face_normal(
	const glm::vec3 &first,
	const glm::vec3 &second,
	const glm::vec3 &third)
{
	return glm::normalize(glm::cross(second - first, third - first));
}

class RevolveMeshConstruction
{
public:
	RevolveMeshConstruction(
		const RevolveShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		std::vector<ProfileTriangle2D> cap_triangles;
		if (!specification_.isFullRevolution() &&
		    (specification_.angularCapPolicy().closesFront() ||
		     specification_.angularCapPolicy().closesBack())) {
			std::string diagnostic;
			if (!ProfileCapTriangulator().triangulate(
					specification_.radialProfile(), &cap_triangles, &diagnostic)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::TriangulationFailure,
					std::move(diagnostic));
			}
		}

		const std::size_t maximum_side_triangles =
			specification_.radialProfile().pointCount() * 2u *
			static_cast<std::size_t>(specification_.angularSegments());
		const std::size_t cap_count = !specification_.isFullRevolution()
			? (specification_.angularCapPolicy().closesFront() ? 1u : 0u) +
			  (specification_.angularCapPolicy().closesBack() ? 1u : 0u)
			: 0u;
		const std::size_t maximum_triangle_count =
			maximum_side_triangles + cap_triangles.size() * cap_count;
		if (maximum_triangle_count >
		        complexity_limits_.maximumGeneratedTriangles() ||
		    maximum_triangle_count >
		        complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"Revolve exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(maximum_triangle_count * 3u);
		mesh_->normals.reserve(maximum_triangle_count * 3u);
		mesh_->texcoords.reserve(maximum_triangle_count * 3u);
		mesh_->faces.reserve(maximum_triangle_count);
		face_surface_tags_.reserve(maximum_triangle_count);

		addLoopSurface(specification_.radialProfile().outerLoop(),
		               MeshSurfaceRole::ProfileOuterSide, 0u);
		for (std::size_t hole_index = 0;
		     hole_index < specification_.radialProfile().innerLoops().size();
		     ++hole_index) {
			addLoopSurface(
				specification_.radialProfile().innerLoops()[hole_index],
				MeshSurfaceRole::ProfileInnerSide,
				hole_index);
		}

		if (!specification_.isFullRevolution()) {
			if (specification_.angularCapPolicy().closesFront()) {
				for (const ProfileTriangle2D &triangle : cap_triangles) {
					addCapTriangle(triangle, startAngle(),
					               MeshSurfaceRole::AngularStart, true);
				}
			}
			if (specification_.angularCapPolicy().closesBack()) {
				for (const ProfileTriangle2D &triangle : cap_triangles) {
					addCapTriangle(triangle, endAngle(),
					               MeshSurfaceRole::AngularEnd, false);
				}
			}
		}

		if (mesh_->faces.empty()) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::DegenerateFace,
				"Revolve produced no nondegenerate triangles.");
		}
		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	float startAngle() const
	{
		return glm::radians(specification_.startDegrees());
	}

	float endAngle() const
	{
		return glm::radians(
			specification_.startDegrees() + specification_.sweepDegrees());
	}

	float angleAt(int angular_index) const
	{
		return glm::radians(
			specification_.startDegrees() +
			specification_.sweepDegrees() *
			static_cast<float>(angular_index) /
			static_cast<float>(specification_.angularSegments()));
	}

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
		const glm::vec3 normal = face_normal(first, second, third);
		const int first_index = addVertex(first, normal, first_texcoord);
		const int second_index = addVertex(second, normal, second_texcoord);
		const int third_index = addVertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, boundary_index);
	}

	void addLoopSurface(const ProfileLoop2D &loop,
	                    MeshSurfaceRole role,
	                    std::size_t boundary_index)
	{
		std::vector<float> profile_distances(loop.points().size() + 1u, 0.0f);
		for (std::size_t point_index = 0;
		     point_index < loop.points().size();
		     ++point_index) {
			profile_distances[point_index + 1u] =
				profile_distances[point_index] +
				glm::length(loop.points()[(point_index + 1u) % loop.points().size()] -
				            loop.points()[point_index]);
		}
		const float perimeter = profile_distances.back();

		for (int angular_index = 0;
		     angular_index < specification_.angularSegments();
		     ++angular_index) {
			const int next_angular_index =
				specification_.isFullRevolution()
					? (angular_index + 1) % specification_.angularSegments()
					: angular_index + 1;
			const float first_angle = angleAt(angular_index);
			const float second_angle = angleAt(next_angular_index);
			const float v0 = static_cast<float>(angular_index) /
			                 static_cast<float>(specification_.angularSegments());
			const float v1 = static_cast<float>(angular_index + 1) /
			                 static_cast<float>(specification_.angularSegments());
			for (std::size_t point_index = 0;
			     point_index < loop.points().size();
			     ++point_index) {
				const std::size_t next_point_index =
					(point_index + 1u) % loop.points().size();
				const glm::vec2 &first_profile = loop.points()[point_index];
				const glm::vec2 &second_profile = loop.points()[next_point_index];
				if (first_profile.x <= kAxisTolerance &&
				    second_profile.x <= kAxisTolerance) {
					continue;
				}
				const glm::vec3 first = position_at(first_profile, first_angle);
				const glm::vec3 second = position_at(second_profile, first_angle);
				const glm::vec3 third = position_at(second_profile, second_angle);
				const glm::vec3 fourth = position_at(first_profile, second_angle);
				const float u0 = profile_distances[point_index] / perimeter;
				const float u1 = profile_distances[point_index + 1u] / perimeter;
				if (first_profile.x <= kAxisTolerance) {
					addTriangle(first, second, third,
					            glm::vec3(u0, v0, 0.0f),
					            glm::vec3(u1, v0, 0.0f),
					            glm::vec3(u1, v1, 0.0f), role, boundary_index);
				}
				else if (second_profile.x <= kAxisTolerance) {
					addTriangle(first, second, fourth,
					            glm::vec3(u0, v0, 0.0f),
					            glm::vec3(u1, v0, 0.0f),
					            glm::vec3(u0, v1, 0.0f), role, boundary_index);
				}
				else {
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
	}

	void addCapTriangle(const ProfileTriangle2D &triangle,
	                    float angle,
	                    MeshSurfaceRole role,
	                    bool reverse)
	{
		const glm::vec3 first = position_at(triangle.first, angle);
		const glm::vec3 second = position_at(triangle.second, angle);
		const glm::vec3 third = position_at(triangle.third, angle);
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

	const RevolveShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
};

}

GeometryBuildResult RevolveMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *revolve =
		dynamic_cast<const RevolveShapeSpecification *>(&specification);
	if (revolve == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"RevolveMeshGenerator requires a RevolveShapeSpecification.");
	}
	return RevolveMeshConstruction(*revolve, complexity_limits_).build();
}

GeneratedPrimitiveMesh RevolveMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
