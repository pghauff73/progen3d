#include "geometry/service/VariableSectionSweepMeshGenerator.h"

#include "geometry/model/VariableSectionSweepShapeSpecification.h"
#include "geometry/service/ProfileCapTriangulator.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace {

constexpr float kMinimumVectorLengthSquared = 1.0e-12f;
constexpr float kDegreesToRadians = 0.017453292519943295769f;

struct VariableSectionSweepFrame
{
	glm::vec3 origin{0.0f};
	glm::vec3 tangent{0.0f, 0.0f, 1.0f};
	glm::vec3 profile_x{1.0f, 0.0f, 0.0f};
	glm::vec3 profile_y{0.0f, 1.0f, 0.0f};
};

struct SampledPathLocation
{
	glm::vec3 origin{0.0f};
	glm::vec3 tangent{0.0f, 0.0f, 1.0f};
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

glm::vec3 calculate_face_normal(
	const glm::vec3 &first,
	const glm::vec3 &second,
	const glm::vec3 &third)
{
	return glm::normalize(glm::cross(second - first, third - first));
}

glm::vec2 transform_station_profile_point(
	const glm::vec2 &point,
	const VariableSectionSweepStationSpecification &station)
{
	const glm::vec2 scaled(point.x * station.scale().x,
	                       point.y * station.scale().y);
	const float angle_radians = station.rotationDegrees() * kDegreesToRadians;
	const float cosine = std::cos(angle_radians);
	const float sine = std::sin(angle_radians);
	return station.center() + glm::vec2(
		scaled.x * cosine - scaled.y * sine,
		scaled.x * sine + scaled.y * cosine);
}

class VariableSectionSweepMeshConstruction
{
public:
	VariableSectionSweepMeshConstruction(
		const VariableSectionSweepShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification),
		  complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		buildPathDistanceTable();
		buildStationFrames();

		std::vector<ProfileTriangle2D> front_cap_triangles;
		std::vector<ProfileTriangle2D> back_cap_triangles;
		if (specification_.capPolicy().closesFront()) {
			std::string diagnostic;
			if (!ProfileCapTriangulator().triangulate(
					specification_.stations().front().profile(),
					&front_cap_triangles,
					&diagnostic)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::TriangulationFailure,
					std::move(diagnostic));
			}
		}
		if (specification_.capPolicy().closesBack()) {
			std::string diagnostic;
			if (!ProfileCapTriangulator().triangulate(
					specification_.stations().back().profile(),
					&back_cap_triangles,
					&diagnostic)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::TriangulationFailure,
					std::move(diagnostic));
			}
		}

		const std::size_t segment_count = specification_.stations().size() - 1u;
		const std::size_t side_triangle_count =
			specification_.stations().front().profile().pointCount() *
			2u * segment_count;
		const std::size_t triangle_count = side_triangle_count +
			front_cap_triangles.size() + back_cap_triangles.size();
		if (triangle_count > complexity_limits_.maximumGeneratedTriangles() ||
		    triangle_count > complexity_limits_.maximumGeneratedVertices() / 3u) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"VariableSectionSweep exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(triangle_count * 3u);
		mesh_->normals.reserve(triangle_count * 3u);
		mesh_->texcoords.reserve(triangle_count * 3u);
		mesh_->faces.reserve(triangle_count);
		face_surface_tags_.reserve(triangle_count);

		addCorrespondingLoopSides(
			0u, MeshSurfaceRole::ProfileOuterSide, 0u);
		for (std::size_t hole_index = 0;
		     hole_index < specification_.stations().front().profile().innerLoops().size();
		     ++hole_index) {
			addCorrespondingLoopSides(
				hole_index + 1u,
				MeshSurfaceRole::ProfileInnerSide,
				hole_index);
		}
		for (const ProfileTriangle2D &triangle : front_cap_triangles) {
			addCapTriangle(
				triangle,
				specification_.stations().front(),
				station_frames_.front(),
				MeshSurfaceRole::ProfileFrontCap,
				true);
		}
		for (const ProfileTriangle2D &triangle : back_cap_triangles) {
			addCapTriangle(
				triangle,
				specification_.stations().back(),
				station_frames_.back(),
				MeshSurfaceRole::ProfileBackCap,
				false);
		}

		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(face_surface_tags_)));
	}

private:
	void buildPathDistanceTable()
	{
		path_distances_.reserve(specification_.pathPoints().size());
		path_distances_.push_back(0.0f);
		for (std::size_t point_index = 1u;
		     point_index < specification_.pathPoints().size();
		     ++point_index) {
			path_distances_.push_back(
				path_distances_.back() +
				glm::length(specification_.pathPoints()[point_index] -
				            specification_.pathPoints()[point_index - 1u]));
		}
	}

	SampledPathLocation samplePath(float normalized_path_position) const
	{
		const float requested_distance =
			normalized_path_position * path_distances_.back();
		auto upper_distance = std::upper_bound(
			path_distances_.begin(), path_distances_.end(), requested_distance);
		std::size_t segment_index = upper_distance == path_distances_.begin()
			? 0u
			: static_cast<std::size_t>(upper_distance - path_distances_.begin() - 1);
		segment_index = std::min(
			segment_index, specification_.pathPoints().size() - 2u);

		const float segment_start_distance = path_distances_[segment_index];
		const float segment_length =
			path_distances_[segment_index + 1u] - segment_start_distance;
		const float segment_position = std::clamp(
			(requested_distance - segment_start_distance) / segment_length,
			0.0f,
			1.0f);
		const glm::vec3 &segment_start = specification_.pathPoints()[segment_index];
		const glm::vec3 &segment_end = specification_.pathPoints()[segment_index + 1u];
		return {
			segment_start + (segment_end - segment_start) * segment_position,
			glm::normalize(segment_end - segment_start)};
	}

	void buildStationFrames()
	{
		station_frames_.reserve(specification_.stations().size());
		if (specification_.framePolicy() == SweepFramePolicy::ReferenceAligned) {
			buildReferenceAlignedStationFrames();
			return;
		}
		for (const VariableSectionSweepStationSpecification &station :
		     specification_.stations()) {
			const SampledPathLocation path_location =
				samplePath(station.normalizedPathPosition());
			glm::vec3 profile_x(0.0f);
			if (station_frames_.empty()) {
				profile_x = glm::cross(
					specification_.upHint(), path_location.tangent);
			}
			else {
				const glm::vec3 &previous_profile_x =
					station_frames_.back().profile_x;
				profile_x = previous_profile_x -
					path_location.tangent *
					glm::dot(previous_profile_x, path_location.tangent);
			}
			if (glm::dot(profile_x, profile_x) <= kMinimumVectorLengthSquared) {
				profile_x = glm::cross(
					least_aligned_axis(path_location.tangent),
					path_location.tangent);
			}
			profile_x = glm::normalize(profile_x);
			if (!station_frames_.empty() &&
			    glm::dot(profile_x, station_frames_.back().profile_x) < 0.0f) {
				profile_x = -profile_x;
			}
			const glm::vec3 profile_y = glm::normalize(
				glm::cross(path_location.tangent, profile_x));
			station_frames_.push_back({
				path_location.origin,
				path_location.tangent,
				profile_x,
				profile_y});
		}
	}

	void buildReferenceAlignedStationFrames()
	{
		const glm::vec3 endpoint_direction =
			specification_.pathPoints().back() - specification_.pathPoints().front();
		const glm::vec3 absolute_direction = glm::abs(endpoint_direction);
		glm::vec3 reference_tangent(0.0f);
		if (absolute_direction.x >= absolute_direction.y &&
		    absolute_direction.x >= absolute_direction.z) {
			reference_tangent.x = endpoint_direction.x < 0.0f ? -1.0f : 1.0f;
		}
		else if (absolute_direction.y >= absolute_direction.z) {
			reference_tangent.y = endpoint_direction.y < 0.0f ? -1.0f : 1.0f;
		}
		else {
			reference_tangent.z = endpoint_direction.z < 0.0f ? -1.0f : 1.0f;
		}
		glm::vec3 profile_x = glm::cross(
			specification_.upHint(), reference_tangent);
		if (glm::dot(profile_x, profile_x) <= kMinimumVectorLengthSquared) {
			profile_x = glm::cross(
				least_aligned_axis(reference_tangent), reference_tangent);
		}
		profile_x = glm::normalize(profile_x);
		const glm::vec3 profile_y = glm::normalize(
			glm::cross(reference_tangent, profile_x));
		for (const VariableSectionSweepStationSpecification &station :
		     specification_.stations()) {
			const SampledPathLocation path_location =
				samplePath(station.normalizedPathPosition());
			station_frames_.push_back({
				path_location.origin,
				path_location.tangent,
				profile_x,
				profile_y});
		}
	}

	const ProfileLoop2D &profileLoop(
		const VariableSectionSweepStationSpecification &station,
		std::size_t loop_index) const
	{
		if (loop_index == 0u) return station.profile().outerLoop();
		return station.profile().innerLoops()[loop_index - 1u];
	}

	glm::vec3 transformPoint(
		const glm::vec2 &profile_point,
		const VariableSectionSweepStationSpecification &station,
		const VariableSectionSweepFrame &frame) const
	{
		const glm::vec2 point = transform_station_profile_point(profile_point, station);
		return frame.origin + frame.profile_x * point.x + frame.profile_y * point.y;
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
		const glm::vec3 &first_texcoord,
		const glm::vec3 &second_texcoord,
		const glm::vec3 &third_texcoord,
		MeshSurfaceRole role,
		std::size_t boundary_index)
	{
		const glm::vec3 normal = calculate_face_normal(first, second, third);
		const int first_index = addVertex(first, normal, first_texcoord);
		const int second_index = addVertex(second, normal, second_texcoord);
		const int third_index = addVertex(third, normal, third_texcoord);
		mesh_->faces.emplace_back(first_index, second_index, third_index);
		face_surface_tags_.emplace_back(role, boundary_index);
	}

	void addCorrespondingLoopSides(
		std::size_t loop_index,
		MeshSurfaceRole role,
		std::size_t boundary_index)
	{
		const ProfileLoop2D &reference_loop =
			profileLoop(specification_.stations().front(), loop_index);
		std::vector<float> profile_distances(
			reference_loop.points().size() + 1u, 0.0f);
		for (std::size_t point_index = 0;
		     point_index < reference_loop.points().size();
		     ++point_index) {
			profile_distances[point_index + 1u] =
				profile_distances[point_index] +
				glm::length(
					reference_loop.points()[
						(point_index + 1u) % reference_loop.points().size()] -
					reference_loop.points()[point_index]);
		}
		const float profile_perimeter = profile_distances.back();

		for (std::size_t station_index = 0;
		     station_index + 1u < specification_.stations().size();
		     ++station_index) {
			const VariableSectionSweepStationSpecification &first_station =
				specification_.stations()[station_index];
			const VariableSectionSweepStationSpecification &second_station =
				specification_.stations()[station_index + 1u];
			const ProfileLoop2D &first_loop = profileLoop(first_station, loop_index);
			const ProfileLoop2D &second_loop = profileLoop(second_station, loop_index);
			for (std::size_t point_index = 0;
			     point_index < first_loop.points().size();
			     ++point_index) {
				const std::size_t next_index =
					(point_index + 1u) % first_loop.points().size();
				const glm::vec3 first = transformPoint(
					first_loop.points()[point_index],
					first_station,
					station_frames_[station_index]);
				const glm::vec3 second = transformPoint(
					first_loop.points()[next_index],
					first_station,
					station_frames_[station_index]);
				const glm::vec3 third = transformPoint(
					second_loop.points()[next_index],
					second_station,
					station_frames_[station_index + 1u]);
				const glm::vec3 fourth = transformPoint(
					second_loop.points()[point_index],
					second_station,
					station_frames_[station_index + 1u]);
				const float first_u = profile_distances[point_index] / profile_perimeter;
				const float second_u = profile_distances[point_index + 1u] / profile_perimeter;
				const float first_v = first_station.normalizedPathPosition();
				const float second_v = second_station.normalizedPathPosition();
				addTriangle(
					first, second, third,
					glm::vec3(first_u, first_v, 0.0f),
					glm::vec3(second_u, first_v, 0.0f),
					glm::vec3(second_u, second_v, 0.0f),
					role, boundary_index);
				addTriangle(
					first, third, fourth,
					glm::vec3(first_u, first_v, 0.0f),
					glm::vec3(second_u, second_v, 0.0f),
					glm::vec3(first_u, second_v, 0.0f),
					role, boundary_index);
			}
		}
	}

	void addCapTriangle(
		const ProfileTriangle2D &triangle,
		const VariableSectionSweepStationSpecification &station,
		const VariableSectionSweepFrame &frame,
		MeshSurfaceRole role,
		bool reverse)
	{
		const glm::vec2 first_profile =
			transform_station_profile_point(triangle.first, station);
		const glm::vec2 second_profile =
			transform_station_profile_point(triangle.second, station);
		const glm::vec2 third_profile =
			transform_station_profile_point(triangle.third, station);
		const glm::vec3 first =
			frame.origin + frame.profile_x * first_profile.x + frame.profile_y * first_profile.y;
		const glm::vec3 second =
			frame.origin + frame.profile_x * second_profile.x + frame.profile_y * second_profile.y;
		const glm::vec3 third =
			frame.origin + frame.profile_x * third_profile.x + frame.profile_y * third_profile.y;
		if (reverse) {
			addTriangle(
				first, third, second,
				glm::vec3(first_profile, 0.0f),
				glm::vec3(third_profile, 0.0f),
				glm::vec3(second_profile, 0.0f),
				role, 0u);
		}
		else {
			addTriangle(
				first, second, third,
				glm::vec3(first_profile, 0.0f),
				glm::vec3(second_profile, 0.0f),
				glm::vec3(third_profile, 0.0f),
				role, 0u);
		}
	}

	const VariableSectionSweepShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> face_surface_tags_;
	std::vector<float> path_distances_;
	std::vector<VariableSectionSweepFrame> station_frames_;
};

}

GeometryBuildResult VariableSectionSweepMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *variable_section_sweep =
		dynamic_cast<const VariableSectionSweepShapeSpecification *>(&specification);
	if (variable_section_sweep == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"VariableSectionSweepMeshGenerator requires a VariableSectionSweepShapeSpecification.");
	}
	return VariableSectionSweepMeshConstruction(
		*variable_section_sweep, complexity_limits_).build();
}

GeneratedPrimitiveMesh VariableSectionSweepMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
