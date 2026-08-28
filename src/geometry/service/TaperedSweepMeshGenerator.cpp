#include "geometry/service/TaperedSweepMeshGenerator.h"

#include "geometry/model/TaperedSweepShapeSpecification.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

struct TaperedSweepFrame
{
	glm::vec3 origin{0.0f};
	glm::vec3 tangent{0.0f, 1.0f, 0.0f};
	glm::vec3 radial_x{1.0f, 0.0f, 0.0f};
	glm::vec3 radial_y{0.0f, 0.0f, 1.0f};
};

glm::vec3 fallback_radial_axis(const glm::vec3 &tangent)
{
	const glm::vec3 reference = std::abs(tangent.y) < 0.9f
		? glm::vec3(0.0f, 1.0f, 0.0f)
		: glm::vec3(1.0f, 0.0f, 0.0f);
	return glm::normalize(glm::cross(reference, tangent));
}

std::vector<TaperedSweepFrame> create_frames(
	const TaperedSweepShapeSpecification &specification)
{
	std::vector<TaperedSweepFrame> frames;
	frames.reserve(specification.pathPoints().size());
	glm::vec3 previous_radial_x(0.0f);
	for (std::size_t index = 0; index < specification.pathPoints().size(); ++index) {
		glm::vec3 tangent;
		if (index == 0u) {
			tangent = specification.pathPoints()[1u] - specification.pathPoints()[0u];
		}
		else if (index + 1u == specification.pathPoints().size()) {
			tangent = specification.pathPoints()[index] -
			          specification.pathPoints()[index - 1u];
		}
		else {
			tangent = specification.pathPoints()[index + 1u] -
			          specification.pathPoints()[index - 1u];
		}
		tangent = glm::normalize(tangent);

		glm::vec3 radial_x;
		if (index == 0u) {
			radial_x = glm::cross(specification.upHint(), tangent);
			if (glm::length(radial_x) <= 1.0e-6f) {
				radial_x = fallback_radial_axis(tangent);
			}
			else {
				radial_x = glm::normalize(radial_x);
			}
		}
		else {
			radial_x = previous_radial_x -
			           tangent * glm::dot(previous_radial_x, tangent);
			if (glm::length(radial_x) <= 1.0e-6f) {
				radial_x = fallback_radial_axis(tangent);
			}
			else {
				radial_x = glm::normalize(radial_x);
			}
		}
		const glm::vec3 radial_y = glm::normalize(glm::cross(tangent, radial_x));
		frames.push_back({specification.pathPoints()[index], tangent, radial_x, radial_y});
		previous_radial_x = radial_x;
	}
	return frames;
}

class TaperedSweepMeshConstruction
{
public:
	explicit TaperedSweepMeshConstruction(
		const TaperedSweepShapeSpecification &specification)
		: specification_(specification),
		  mesh_(std::make_shared<Mesh>()),
		  frames_(create_frames(specification))
	{
	}

	GeometryBuildResult build()
	{
		const std::size_t segment_count =
			static_cast<std::size_t>(specification_.circumferentialSegments());
		const std::size_t side_triangle_count =
			(frames_.size() - 1u) * segment_count * 2u;
		const std::size_t cap_triangle_count =
			(specification_.capPolicy().closesFront() ? segment_count : 0u) +
			(specification_.capPolicy().closesBack() ? segment_count : 0u);
		const std::size_t triangle_count =
			side_triangle_count + cap_triangle_count;
		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);

		std::vector<float> path_distances(frames_.size(), 0.0f);
		for (std::size_t index = 1u; index < frames_.size(); ++index) {
			path_distances[index] = path_distances[index - 1u] +
				glm::length(frames_[index].origin - frames_[index - 1u].origin);
		}
		const float total_path_length = path_distances.back();
		for (std::size_t path_index = 0;
		     path_index + 1u < frames_.size();
		     ++path_index) {
			for (std::size_t angular_index = 0;
			     angular_index < segment_count;
			     ++angular_index) {
				const std::size_t next_angular =
					(angular_index + 1u) % segment_count;
				const glm::vec3 first = ring_point(path_index, angular_index);
				const glm::vec3 second = ring_point(path_index, next_angular);
				const glm::vec3 third = ring_point(path_index + 1u, next_angular);
				const glm::vec3 fourth = ring_point(path_index + 1u, angular_index);
				const float u0 = static_cast<float>(angular_index) /
				                 static_cast<float>(segment_count);
				const float u1 = static_cast<float>(angular_index + 1u) /
				                 static_cast<float>(segment_count);
				const float v0 = path_distances[path_index] / total_path_length;
				const float v1 = path_distances[path_index + 1u] / total_path_length;
				add_triangle(
					first, second, third,
					glm::vec3(u0, v0, 0.0f), glm::vec3(u1, v0, 0.0f),
					glm::vec3(u1, v1, 0.0f),
					MeshSurfaceRole::BotanicalBranchSide);
				add_triangle(
					first, third, fourth,
					glm::vec3(u0, v0, 0.0f), glm::vec3(u1, v1, 0.0f),
					glm::vec3(u0, v1, 0.0f),
					MeshSurfaceRole::BotanicalBranchSide);
			}
		}

		if (specification_.capPolicy().closesFront()) {
			add_cap(0u, true, MeshSurfaceRole::BotanicalBranchBase);
		}
		if (specification_.capPolicy().closesBack()) {
			add_cap(
				frames_.size() - 1u, false,
				MeshSurfaceRole::BotanicalBranchTip);
		}
		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	glm::vec3 ring_point(std::size_t path_index, std::size_t angular_index) const
	{
		const float angle = glm::two_pi<float>() *
			static_cast<float>(angular_index) /
			static_cast<float>(specification_.circumferentialSegments());
		const TaperedSweepFrame &frame = frames_[path_index];
		return frame.origin + specification_.radii()[path_index] *
			(frame.radial_x * std::cos(angle) + frame.radial_y * std::sin(angle));
	}

	int add_vertex(
		const glm::vec3 &position,
		const glm::vec3 &normal,
		const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->normals.push_back(normal);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1u);
	}

	void add_triangle(
		const glm::vec3 &first,
		const glm::vec3 &second,
		const glm::vec3 &third,
		const glm::vec3 &first_texcoord,
		const glm::vec3 &second_texcoord,
		const glm::vec3 &third_texcoord,
		MeshSurfaceRole role)
	{
		const glm::vec3 cross = glm::cross(second - first, third - first);
		const glm::vec3 normal = glm::normalize(cross);
		const int first_index = add_vertex(first, normal, first_texcoord);
		const int second_index = add_vertex(second, normal, second_texcoord);
		const int third_index = add_vertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, 0u);
	}

	void add_cap(
		std::size_t path_index,
		bool reverse,
		MeshSurfaceRole role)
	{
		const std::size_t segment_count =
			static_cast<std::size_t>(specification_.circumferentialSegments());
		for (std::size_t angular_index = 0;
		     angular_index < segment_count;
		     ++angular_index) {
			const std::size_t next_angular =
				(angular_index + 1u) % segment_count;
			const glm::vec3 current = ring_point(path_index, angular_index);
			const glm::vec3 next = ring_point(path_index, next_angular);
			if (reverse) {
				add_triangle(
					frames_[path_index].origin, next, current,
					glm::vec3(0.5f, 0.5f, 0.0f),
					glm::vec3(1.0f, 1.0f, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f), role);
			}
			else {
				add_triangle(
					frames_[path_index].origin, current, next,
					glm::vec3(0.5f, 0.5f, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f),
					glm::vec3(1.0f, 1.0f, 0.0f), role);
			}
		}
	}

	const TaperedSweepShapeSpecification &specification_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
	std::vector<TaperedSweepFrame> frames_;
};

} // namespace

GeometryBuildResult TaperedSweepMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *tapered =
		dynamic_cast<const TaperedSweepShapeSpecification *>(&specification);
	if (tapered == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"TaperedSweepMeshGenerator requires a TaperedSweepShapeSpecification.");
	}
	return TaperedSweepMeshConstruction(*tapered).build();
}

GeneratedPrimitiveMesh TaperedSweepMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}

