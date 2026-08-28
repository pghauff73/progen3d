#include "geometry/service/SweepProfileMeshGenerator.h"

#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/service/ProfileCapTriangulator.h"

#include <glm/geometric.hpp>

#include <memory>
#include <utility>

namespace {

struct SweepFrame
{
	glm::vec3 origin{0.0f};
	glm::vec3 tangent{0.0f, 0.0f, 1.0f};
	glm::vec3 profile_x{1.0f, 0.0f, 0.0f};
	glm::vec3 profile_y{0.0f, 1.0f, 0.0f};
};

glm::vec3 least_aligned_axis(const glm::vec3 &direction)
{
	const glm::vec3 absolute = glm::abs(direction);
	if (absolute.x <= absolute.y && absolute.x <= absolute.z) {
		return glm::vec3(1.0f, 0.0f, 0.0f);
	}
	if (absolute.y <= absolute.z) return glm::vec3(0.0f, 1.0f, 0.0f);
	return glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::vec3 face_normal(
	const glm::vec3 &first,
	const glm::vec3 &second,
	const glm::vec3 &third)
{
	return glm::normalize(glm::cross(second - first, third - first));
}

class SweepProfileMeshConstruction
{
public:
	SweepProfileMeshConstruction(
		const SweepProfileShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		buildFrames();
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
			specification_.profile().pointCount() * 2u *
			(specification_.pathPoints().size() - 1u);
		const std::size_t cap_count =
			(specification_.capPolicy().closesFront() ? 1u : 0u) +
			(specification_.capPolicy().closesBack() ? 1u : 0u);
		const std::size_t triangle_count =
			side_triangle_count + cap_triangles.size() * cap_count;
		if (triangle_count > complexity_limits_.maximumGeneratedTriangles() ||
		    triangle_count > complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"SweepProfile exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);

		addLoopSides(specification_.profile().outerLoop(),
		             MeshSurfaceRole::ProfileOuterSide, 0u);
		for (std::size_t hole_index = 0;
		     hole_index < specification_.profile().innerLoops().size();
		     ++hole_index) {
			addLoopSides(specification_.profile().innerLoops()[hole_index],
			             MeshSurfaceRole::ProfileInnerSide, hole_index);
		}
		if (specification_.capPolicy().closesFront()) {
			for (const ProfileTriangle2D &triangle : cap_triangles) {
				addCapTriangle(triangle, frames_.front(),
				               MeshSurfaceRole::ProfileFrontCap, true);
			}
		}
		if (specification_.capPolicy().closesBack()) {
			for (const ProfileTriangle2D &triangle : cap_triangles) {
				addCapTriangle(triangle, frames_.back(),
				               MeshSurfaceRole::ProfileBackCap, false);
			}
		}

		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	void buildFrames()
	{
		frames_.reserve(specification_.pathPoints().size());
		path_distances_.push_back(0.0f);
		for (std::size_t point_index = 0;
		     point_index < specification_.pathPoints().size();
		     ++point_index) {
			glm::vec3 tangent(0.0f);
			if (point_index == 0u) {
				tangent = specification_.pathPoints()[1] -
				          specification_.pathPoints()[0];
			}
			else if (point_index + 1u == specification_.pathPoints().size()) {
				tangent = specification_.pathPoints()[point_index] -
				          specification_.pathPoints()[point_index - 1u];
			}
			else {
				const glm::vec3 incoming = glm::normalize(
					specification_.pathPoints()[point_index] -
					specification_.pathPoints()[point_index - 1u]);
				const glm::vec3 outgoing = glm::normalize(
					specification_.pathPoints()[point_index + 1u] -
					specification_.pathPoints()[point_index]);
				tangent = incoming + outgoing;
				if (glm::dot(tangent, tangent) <= 1.0e-12f) tangent = outgoing;
			}
			tangent = glm::normalize(tangent);
			glm::vec3 profile_x = glm::cross(specification_.upHint(), tangent);
			if (glm::dot(profile_x, profile_x) <= 1.0e-12f) {
				profile_x = glm::cross(least_aligned_axis(tangent), tangent);
			}
			profile_x = glm::normalize(profile_x);
			const glm::vec3 profile_y = glm::normalize(
				glm::cross(tangent, profile_x));
			frames_.push_back({specification_.pathPoints()[point_index],
			                   tangent, profile_x, profile_y});
			if (point_index > 0u) {
				path_distances_.push_back(
					path_distances_.back() +
					glm::length(specification_.pathPoints()[point_index] -
					            specification_.pathPoints()[point_index - 1u]));
			}
		}
	}

	glm::vec3 transformPoint(const glm::vec2 &point, const SweepFrame &frame) const
	{
		return frame.origin + frame.profile_x * point.x + frame.profile_y * point.y;
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
		const glm::vec3 normal = face_normal(first, second, third);
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
		std::vector<float> profile_distances(loop.points().size() + 1u, 0.0f);
		for (std::size_t point_index = 0;
		     point_index < loop.points().size();
		     ++point_index) {
			profile_distances[point_index + 1u] =
				profile_distances[point_index] +
				glm::length(loop.points()[(point_index + 1u) % loop.points().size()] -
				            loop.points()[point_index]);
		}
		const float profile_perimeter = profile_distances.back();
		const float path_length = path_distances_.back();

		for (std::size_t path_index = 0;
		     path_index + 1u < frames_.size();
		     ++path_index) {
			for (std::size_t point_index = 0;
			     point_index < loop.points().size();
			     ++point_index) {
				const std::size_t next_index =
					(point_index + 1u) % loop.points().size();
				const glm::vec3 first =
					transformPoint(loop.points()[point_index], frames_[path_index]);
				const glm::vec3 second =
					transformPoint(loop.points()[next_index], frames_[path_index]);
				const glm::vec3 third =
					transformPoint(loop.points()[next_index], frames_[path_index + 1u]);
				const glm::vec3 fourth =
					transformPoint(loop.points()[point_index], frames_[path_index + 1u]);
				const float u0 = profile_distances[point_index] / profile_perimeter;
				const float u1 = profile_distances[point_index + 1u] / profile_perimeter;
				const float v0 = path_distances_[path_index] / path_length;
				const float v1 = path_distances_[path_index + 1u] / path_length;
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

	void addCapTriangle(const ProfileTriangle2D &triangle,
	                    const SweepFrame &frame,
	                    MeshSurfaceRole role,
	                    bool reverse)
	{
		const glm::vec3 first = transformPoint(triangle.first, frame);
		const glm::vec3 second = transformPoint(triangle.second, frame);
		const glm::vec3 third = transformPoint(triangle.third, frame);
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

	const SweepProfileShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
	std::vector<SweepFrame> frames_;
	std::vector<float> path_distances_;
};

}

GeometryBuildResult SweepProfileMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *sweep =
		dynamic_cast<const SweepProfileShapeSpecification *>(&specification);
	if (sweep == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"SweepProfileMeshGenerator requires a SweepProfileShapeSpecification.");
	}
	return SweepProfileMeshConstruction(*sweep, complexity_limits_).build();
}

GeneratedPrimitiveMesh SweepProfileMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
