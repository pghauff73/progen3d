#include "geometry/service/SweepDiskMeshGenerator.h"

#include "geometry/model/SweepDiskShapeSpecification.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

constexpr float frame_tolerance = 1.0e-7f;

struct SweepDiskFrame
{
	glm::vec3 origin{0.0f};
	glm::vec3 tangent{0.0f, 1.0f, 0.0f};
	glm::vec3 radial_x{1.0f, 0.0f, 0.0f};
	glm::vec3 radial_y{0.0f, 0.0f, 1.0f};
	float radius = 0.0f;
	float normalized_distance = 0.0f;
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

glm::vec3 rotate_around_axis(
	const glm::vec3 &value,
	const glm::vec3 &axis,
	float cosine,
	float sine)
{
	return value * cosine + glm::cross(axis, value) * sine +
	       axis * glm::dot(axis, value) * (1.0f - cosine);
}

glm::vec3 calculate_tangent(
	const std::vector<glm::vec3> &points,
	std::size_t index)
{
	if (index == 0u) return glm::normalize(points[1u] - points[0u]);
	if (index + 1u == points.size()) {
		return glm::normalize(points[index] - points[index - 1u]);
	}
	const glm::vec3 incoming = glm::normalize(points[index] - points[index - 1u]);
	const glm::vec3 outgoing = glm::normalize(points[index + 1u] - points[index]);
	glm::vec3 tangent = incoming + outgoing;
	if (glm::dot(tangent, tangent) <= frame_tolerance * frame_tolerance) {
		tangent = outgoing;
	}
	return glm::normalize(tangent);
}

std::vector<SweepDiskFrame> build_frames(
	const SweepDiskShapeSpecification &specification,
	std::string *diagnostic)
{
	const std::vector<glm::vec3> &points = specification.pathPoints();
	std::vector<float> distances(points.size(), 0.0f);
	for (std::size_t index = 1u; index < points.size(); ++index) {
		const float segment_length = glm::length(points[index] - points[index - 1u]);
		if (!std::isfinite(segment_length) || segment_length <= frame_tolerance) {
			if (diagnostic != nullptr) {
				*diagnostic = "SweepDisk sampled curve contains a zero-length segment.";
			}
			return {};
		}
		distances[index] = distances[index - 1u] + segment_length;
	}
	const float total_length = distances.back();
	if (!std::isfinite(total_length) || total_length <= frame_tolerance) {
		if (diagnostic != nullptr) *diagnostic = "SweepDisk curve length is zero.";
		return {};
	}

	std::vector<SweepDiskFrame> frames;
	frames.reserve(points.size());
	const glm::vec3 first_tangent = calculate_tangent(points, 0u);
	glm::vec3 first_radial_x = glm::cross(specification.upHint(), first_tangent);
	if (glm::dot(first_radial_x, first_radial_x) <=
	    frame_tolerance * frame_tolerance) {
		first_radial_x = glm::cross(least_aligned_axis(first_tangent), first_tangent);
	}
	first_radial_x = glm::normalize(first_radial_x);
	glm::vec3 first_radial_y = glm::normalize(
		glm::cross(first_tangent, first_radial_x));
	frames.push_back({
		points.front(), first_tangent, first_radial_x, first_radial_y,
		specification.radiusStart(), 0.0f});

	for (std::size_t index = 1u; index < points.size(); ++index) {
		const glm::vec3 tangent = calculate_tangent(points, index);
		const SweepDiskFrame &previous = frames.back();
		const glm::vec3 rotation_axis_vector = glm::cross(previous.tangent, tangent);
		const float sine = glm::length(rotation_axis_vector);
		const float cosine = std::clamp(glm::dot(previous.tangent, tangent), -1.0f, 1.0f);
		if (sine <= frame_tolerance && cosine < 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "SweepDisk curve contains a 180-degree tangent reversal.";
			}
			return {};
		}
		glm::vec3 radial_x = previous.radial_x;
		if (sine > frame_tolerance) {
			radial_x = rotate_around_axis(
				radial_x, rotation_axis_vector / sine, cosine, sine);
		}
		radial_x -= tangent * glm::dot(radial_x, tangent);
		if (glm::dot(radial_x, radial_x) <= frame_tolerance * frame_tolerance) {
			radial_x = glm::cross(least_aligned_axis(tangent), tangent);
		}
		radial_x = glm::normalize(radial_x);
		const glm::vec3 radial_y = glm::normalize(glm::cross(tangent, radial_x));
		const float fraction = distances[index] / total_length;
		const float radius = specification.radiusStart() +
		                     (specification.radiusEnd() - specification.radiusStart()) *
		                     fraction;
		frames.push_back({
			points[index], tangent, radial_x, radial_y, radius, fraction});
	}
	return frames;
}

class SweepDiskMeshConstruction
{
public:
	SweepDiskMeshConstruction(
		const SweepDiskShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		std::string diagnostic;
		frames_ = build_frames(specification_, &diagnostic);
		if (frames_.size() < 2u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::InvalidSpecification, std::move(diagnostic));
		}
		const std::size_t radial_segments = static_cast<std::size_t>(
			specification_.circumferentialSegments());
		const std::size_t side_triangles =
			(frames_.size() - 1u) * radial_segments * 2u;
		const std::size_t cap_count =
			(specification_.capPolicy().closesFront() ? 1u : 0u) +
			(specification_.capPolicy().closesBack() ? 1u : 0u);
		const std::size_t triangle_count = side_triangles + cap_count * radial_segments;
		if (triangle_count > complexity_limits_.maximumGeneratedTriangles() ||
		    triangle_count > complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"SweepDisk exceeds the configured generated mesh limits.");
		}
		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);
		addSides();
		if (specification_.capPolicy().closesFront()) addCap(frames_.front(), true);
		if (specification_.capPolicy().closesBack()) addCap(frames_.back(), false);
		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	glm::vec3 ringPoint(const SweepDiskFrame &frame, float angle) const
	{
		return frame.origin + frame.radius *
		       (frame.radial_x * std::cos(angle) + frame.radial_y * std::sin(angle));
	}

	int addVertex(
		const glm::vec3 &position,
		const glm::vec3 &normal,
		const glm::vec3 &texcoord)
	{
		mesh_->vertices.push_back(position);
		mesh_->normals.push_back(normal);
		mesh_->texcoords.push_back(texcoord);
		return static_cast<int>(mesh_->vertices.size() - 1u);
	}

	void addTriangle(
		const glm::vec3 &first,
		const glm::vec3 &second,
		const glm::vec3 &third,
		const glm::vec3 &first_uv,
		const glm::vec3 &second_uv,
		const glm::vec3 &third_uv,
		MeshSurfaceRole role)
	{
		const glm::vec3 cross = glm::cross(second - first, third - first);
		if (glm::dot(cross, cross) <= frame_tolerance * frame_tolerance) return;
		const glm::vec3 normal = glm::normalize(cross);
		const int first_index = addVertex(first, normal, first_uv);
		const int second_index = addVertex(second, normal, second_uv);
		const int third_index = addVertex(third, normal, third_uv);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, 0u);
	}

	void addSides()
	{
		const std::size_t radial_segments = static_cast<std::size_t>(
			specification_.circumferentialSegments());
		for (std::size_t path_index = 0u;
		     path_index + 1u < frames_.size();
		     ++path_index) {
			for (std::size_t radial_index = 0u;
			     radial_index < radial_segments;
			     ++radial_index) {
				const std::size_t next_radial = (radial_index + 1u) % radial_segments;
				const float angle0 = glm::two_pi<float>() *
				                     static_cast<float>(radial_index) /
				                     static_cast<float>(radial_segments);
				const float angle1 = glm::two_pi<float>() *
				                     static_cast<float>(next_radial) /
				                     static_cast<float>(radial_segments);
				const glm::vec3 first = ringPoint(frames_[path_index], angle0);
				const glm::vec3 second = ringPoint(frames_[path_index], angle1);
				const glm::vec3 third = ringPoint(frames_[path_index + 1u], angle1);
				const glm::vec3 fourth = ringPoint(frames_[path_index + 1u], angle0);
				const float v0 = static_cast<float>(radial_index) /
				                 static_cast<float>(radial_segments);
				const float v1 = static_cast<float>(radial_index + 1u) /
				                 static_cast<float>(radial_segments);
				addTriangle(
					first, second, third,
					glm::vec3(frames_[path_index].normalized_distance, v0, 0.0f),
					glm::vec3(frames_[path_index].normalized_distance, v1, 0.0f),
					glm::vec3(frames_[path_index + 1u].normalized_distance, v1, 0.0f),
					MeshSurfaceRole::ProfileOuterSide);
				addTriangle(
					first, third, fourth,
					glm::vec3(frames_[path_index].normalized_distance, v0, 0.0f),
					glm::vec3(frames_[path_index + 1u].normalized_distance, v1, 0.0f),
					glm::vec3(frames_[path_index + 1u].normalized_distance, v0, 0.0f),
					MeshSurfaceRole::ProfileOuterSide);
			}
		}
	}

	void addCap(const SweepDiskFrame &frame, bool reverse)
	{
		const std::size_t radial_segments = static_cast<std::size_t>(
			specification_.circumferentialSegments());
		for (std::size_t radial_index = 0u;
		     radial_index < radial_segments;
		     ++radial_index) {
			const std::size_t next_radial = (radial_index + 1u) % radial_segments;
			const float angle0 = glm::two_pi<float>() *
			                     static_cast<float>(radial_index) /
			                     static_cast<float>(radial_segments);
			const float angle1 = glm::two_pi<float>() *
			                     static_cast<float>(next_radial) /
			                     static_cast<float>(radial_segments);
			const glm::vec3 first = ringPoint(frame, angle0);
			const glm::vec3 second = ringPoint(frame, angle1);
			const MeshSurfaceRole role = reverse
				? MeshSurfaceRole::ProfileFrontCap
				: MeshSurfaceRole::ProfileBackCap;
			if (reverse) {
				addTriangle(
					frame.origin, second, first,
					glm::vec3(0.5f, 0.5f, 0.0f),
					glm::vec3(0.5f + 0.5f * std::cos(angle1),
					          0.5f + 0.5f * std::sin(angle1), 0.0f),
					glm::vec3(0.5f + 0.5f * std::cos(angle0),
					          0.5f + 0.5f * std::sin(angle0), 0.0f), role);
			}
			else {
				addTriangle(
					frame.origin, first, second,
					glm::vec3(0.5f, 0.5f, 0.0f),
					glm::vec3(0.5f + 0.5f * std::cos(angle0),
					          0.5f + 0.5f * std::sin(angle0), 0.0f),
					glm::vec3(0.5f + 0.5f * std::cos(angle1),
					          0.5f + 0.5f * std::sin(angle1), 0.0f), role);
			}
		}
	}

	const SweepDiskShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
	std::vector<SweepDiskFrame> frames_;
};

}

GeometryBuildResult SweepDiskMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *disk = dynamic_cast<const SweepDiskShapeSpecification *>(&specification);
	if (disk == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"SweepDiskMeshGenerator requires a SweepDiskShapeSpecification.");
	}
	return SweepDiskMeshConstruction(*disk, complexity_limits_).build();
}

GeneratedPrimitiveMesh SweepDiskMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
