#include "vehicle/mcsmv2/service/SemanticImplicitAgreementEvaluationService.h"

#include "vehicle/mcsmv2/service/VehicleSurfaceProjectionService.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {

std::vector<glm::dvec3> calculateVertexNormals(const Mesh &mesh)
{
	std::vector<glm::dvec3> normals(mesh.vertices.size(), glm::dvec3(0.0));
	for (const glm::ivec3 &face : mesh.faces) {
		if (face.x < 0 || face.y < 0 || face.z < 0 ||
		    static_cast<std::size_t>(face.x) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.y) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.z) >= mesh.vertices.size()) {
			continue;
		}
		const glm::dvec3 first(mesh.vertices[static_cast<std::size_t>(face.x)]);
		const glm::dvec3 second(mesh.vertices[static_cast<std::size_t>(face.y)]);
		const glm::dvec3 third(mesh.vertices[static_cast<std::size_t>(face.z)]);
		const glm::dvec3 unnormalized_face_normal =
			glm::cross(second - first, third - first);
		const double squared_face_normal_length =
			glm::dot(unnormalized_face_normal, unnormalized_face_normal);
		if (squared_face_normal_length <= 1.0e-24) continue;
		const glm::dvec3 face_normal =
			unnormalized_face_normal / std::sqrt(squared_face_normal_length);
		auto cornerAngle = [](const glm::dvec3 &first_edge,
		                      const glm::dvec3 &second_edge) {
			const double first_length = std::sqrt(glm::dot(first_edge, first_edge));
			const double second_length = std::sqrt(glm::dot(second_edge, second_edge));
			if (first_length <= 1.0e-12 || second_length <= 1.0e-12) return 0.0;
			return std::acos(std::clamp(
				glm::dot(first_edge, second_edge) /
					(first_length * second_length),
				-1.0,
				1.0));
		};
		normals[static_cast<std::size_t>(face.x)] +=
			face_normal * cornerAngle(second - first, third - first);
		normals[static_cast<std::size_t>(face.y)] +=
			face_normal * cornerAngle(first - second, third - second);
		normals[static_cast<std::size_t>(face.z)] +=
			face_normal * cornerAngle(first - third, second - third);
	}
	for (glm::dvec3 &normal : normals) {
		const double squared_length = glm::dot(normal, normal);
		normal = squared_length > 1.0e-24
			? normal / std::sqrt(squared_length)
			: glm::dvec3(0.0, 1.0, 0.0);
	}
	return normals;
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

std::vector<std::size_t> sampleIndices(std::size_t count, std::size_t maximum)
{
	if (maximum == 0u || count == 0u) return {};
	if (maximum == 1u) return {0u};
	if (count <= maximum) {
		std::vector<std::size_t> indices(count);
		std::iota(indices.begin(), indices.end(), 0u);
		return indices;
	}
	std::vector<std::size_t> indices;
	indices.reserve(maximum);
	for (std::size_t sample = 0u; sample < maximum; ++sample) {
		const std::size_t index = static_cast<std::size_t>(std::llround(
			static_cast<double>(sample) * static_cast<double>(count - 1u) /
			static_cast<double>(maximum - 1u)));
		if (indices.empty() || indices.back() != index) indices.push_back(index);
	}
	return indices;
}

enum class LongitudinalCorrespondenceRegion
{
	All,
	Rear,
	Cabin,
	Front
};

struct SurfaceCorrespondenceSample
{
	double distance = 0.0;
	double normal_angle_degrees = 0.0;
	double normal_dot = 1.0;
	double longitudinal_coordinate = 0.0;
	glm::dvec3 closest_point{0.0};
};

bool belongsToRegion(
	double longitudinal_coordinate,
	LongitudinalCorrespondenceRegion region)
{
	if (region == LongitudinalCorrespondenceRegion::All) return true;
	if (region == LongitudinalCorrespondenceRegion::Rear) {
		return longitudinal_coordinate < (1.0 / 3.0);
	}
	if (region == LongitudinalCorrespondenceRegion::Cabin) {
		return longitudinal_coordinate >= (1.0 / 3.0) &&
		       longitudinal_coordinate < (2.0 / 3.0);
	}
	return longitudinal_coordinate >= (2.0 / 3.0);
}

const char *regionIdentifier(LongitudinalCorrespondenceRegion region)
{
	if (region == LongitudinalCorrespondenceRegion::Rear) return "rear";
	if (region == LongitudinalCorrespondenceRegion::Cabin) return "cabin";
	if (region == LongitudinalCorrespondenceRegion::Front) return "front";
	return "all";
}

double percentile(std::vector<double> values, double fraction)
{
	if (values.empty()) return 0.0;
	std::sort(values.begin(), values.end());
	const double position = fraction * static_cast<double>(values.size() - 1u);
	const std::size_t lower = static_cast<std::size_t>(std::floor(position));
	const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
	const double blend = position - static_cast<double>(lower);
	return values[lower] * (1.0 - blend) + values[upper] * blend;
}

void hashQuantizedPoint(std::uint64_t *hash, const glm::dvec3 &point)
{
	for (const double component : {point.x, point.y, point.z}) {
		const std::int64_t quantized =
			static_cast<std::int64_t>(std::llround(component * 100000000.0));
		const auto *bytes = reinterpret_cast<const unsigned char *>(&quantized);
		for (std::size_t byte_index = 0u; byte_index < sizeof(quantized); ++byte_index) {
			*hash ^= bytes[byte_index];
			*hash *= 1099511628211ull;
		}
	}
}

std::string hexadecimalHash(std::uint64_t hash)
{
	std::ostringstream stream;
	stream << std::hex << std::setfill('0') << std::setw(16) << hash;
	return stream.str();
}

std::vector<SurfaceCorrespondenceSample> evaluateSamples(
	const Mesh &source,
	const Mesh &target,
	std::size_t maximum_samples,
	const std::vector<glm::dvec2> *source_surface_coordinates,
	const std::vector<glm::dvec2> *target_surface_coordinates)
{
	const std::vector<glm::dvec3> source_normals = calculateVertexNormals(source);
	const std::vector<glm::dvec3> target_normals = calculateVertexNormals(target);
	const std::vector<std::size_t> sample_indices =
		sampleIndices(source.vertices.size(), maximum_samples);
	std::vector<SurfaceCorrespondenceSample> samples;
	samples.reserve(sample_indices.size());
	VehicleSurfaceProjectionService projection_service;
	for (const std::size_t source_vertex_index : sample_indices) {
		const glm::dvec3 query_point(source.vertices[source_vertex_index]);
		const std::optional<VehicleSurfaceProjection> projection =
			projection_service.projectPoint(target, query_point);
		if (!projection) {
			throw std::runtime_error(
				"Semantic-to-implicit correspondence projection failed.");
		}
		const glm::ivec3 &target_face = target.faces[projection->faceIndex()];
		const glm::dvec3 barycentric = projection->barycentricCoordinates();
		glm::dvec3 target_normal =
			barycentric.x * target_normals[static_cast<std::size_t>(target_face.x)] +
			barycentric.y * target_normals[static_cast<std::size_t>(target_face.y)] +
			barycentric.z * target_normals[static_cast<std::size_t>(target_face.z)];
		const double target_normal_squared_length = glm::dot(target_normal, target_normal);
		target_normal = target_normal_squared_length > 1.0e-24
			? target_normal / std::sqrt(target_normal_squared_length)
			: calculateFaceNormal(target, projection->faceIndex());
		const double normal_dot = std::clamp(
			glm::dot(source_normals[source_vertex_index], target_normal), -1.0, 1.0);
		const double angle_degrees =
			std::acos(normal_dot) * 180.0 / 3.14159265358979323846;
		double longitudinal_coordinate = 0.5;
		if (source_surface_coordinates != nullptr) {
			longitudinal_coordinate =
				source_surface_coordinates->at(source_vertex_index).x;
		}
		else if (target_surface_coordinates != nullptr) {
			longitudinal_coordinate =
				barycentric.x * target_surface_coordinates->at(
					static_cast<std::size_t>(target_face.x)).x +
				barycentric.y * target_surface_coordinates->at(
					static_cast<std::size_t>(target_face.y)).x +
				barycentric.z * target_surface_coordinates->at(
					static_cast<std::size_t>(target_face.z)).x;
		}
		samples.push_back(SurfaceCorrespondenceSample{
			projection->distance(),
			angle_degrees,
			normal_dot,
			std::clamp(longitudinal_coordinate, 0.0, 1.0),
			projection->projectedPoint()});
	}
	return samples;
}

SurfaceCorrespondenceDirectionReport summarizeSamples(
	const std::vector<SurfaceCorrespondenceSample> &samples,
	LongitudinalCorrespondenceRegion region)
{
	std::vector<double> distances;
	std::vector<double> normal_angles;
	double distance_sum = 0.0;
	double squared_distance_sum = 0.0;
	double minimum_normal_dot = 1.0;
	std::uint64_t closest_point_hash = 1469598103934665603ull;
	for (const SurfaceCorrespondenceSample &sample : samples) {
		if (!belongsToRegion(sample.longitudinal_coordinate, region)) continue;
		distances.push_back(sample.distance);
		normal_angles.push_back(sample.normal_angle_degrees);
		distance_sum += sample.distance;
		squared_distance_sum += sample.distance * sample.distance;
		minimum_normal_dot = std::min(minimum_normal_dot, sample.normal_dot);
		hashQuantizedPoint(&closest_point_hash, sample.closest_point);
	}
	const double sample_count = static_cast<double>(distances.size());
	return SurfaceCorrespondenceDirectionReport(
		distances.size(),
		distances.empty() ? 0.0 : distance_sum / sample_count,
		distances.empty() ? 0.0 : std::sqrt(squared_distance_sum / sample_count),
		percentile(distances, 0.95),
		distances.empty() ? 0.0 : *std::max_element(distances.begin(), distances.end()),
		normal_angles.empty()
			? 0.0
			: std::accumulate(normal_angles.begin(), normal_angles.end(), 0.0) /
				  sample_count,
		percentile(normal_angles, 0.95),
		normal_angles.empty()
			? 0.0
			: *std::max_element(normal_angles.begin(), normal_angles.end()),
		minimum_normal_dot,
		hexadecimalHash(closest_point_hash));
}

bool directionPassed(
	const SurfaceCorrespondenceDirectionReport &direction,
	const SemanticImplicitCorrespondenceReport &accepted)
{
	return direction.percentile95Distance() <= accepted.percentile95DistanceTolerance() &&
	       direction.maximumDistance() <= accepted.maximumDistanceTolerance() &&
	       direction.percentile95NormalAngleDegrees() <=
		       accepted.percentile95NormalAngleToleranceDegrees();
}

} // namespace

SemanticImplicitCorrespondenceReport
SemanticImplicitAgreementEvaluationService::evaluate(
	const Mesh &registered_semantic_surface,
	const std::vector<glm::dvec2> &registered_vertex_surface_coordinates,
	const Mesh &implicit_scaffold_surface,
	const SemanticImplicitCorrespondenceReport &accepted_correspondence,
	std::size_t maximum_samples) const
{
	if (registered_semantic_surface.vertices.size() !=
	    registered_vertex_surface_coordinates.size()) {
		throw std::invalid_argument(
			"Registered semantic surface UV count must equal its vertex count.");
	}
	const std::vector<SurfaceCorrespondenceSample> semantic_to_scaffold_samples =
		evaluateSamples(
			registered_semantic_surface,
			implicit_scaffold_surface,
			maximum_samples,
			&registered_vertex_surface_coordinates,
			nullptr);
	const std::vector<SurfaceCorrespondenceSample> scaffold_to_semantic_samples =
		evaluateSamples(
			implicit_scaffold_surface,
			registered_semantic_surface,
			maximum_samples,
			nullptr,
			&registered_vertex_surface_coordinates);
	const SurfaceCorrespondenceDirectionReport semantic_to_scaffold =
		summarizeSamples(
			semantic_to_scaffold_samples, LongitudinalCorrespondenceRegion::All);
	const SurfaceCorrespondenceDirectionReport scaffold_to_semantic =
		summarizeSamples(
			scaffold_to_semantic_samples, LongitudinalCorrespondenceRegion::All);
	std::vector<SemanticImplicitRegionCorrespondenceReport> region_reports;
	bool regions_passed = true;
	for (const LongitudinalCorrespondenceRegion region : {
		     LongitudinalCorrespondenceRegion::Rear,
		     LongitudinalCorrespondenceRegion::Cabin,
		     LongitudinalCorrespondenceRegion::Front}) {
		const SurfaceCorrespondenceDirectionReport semantic_region =
			summarizeSamples(semantic_to_scaffold_samples, region);
		const SurfaceCorrespondenceDirectionReport scaffold_region =
			summarizeSamples(scaffold_to_semantic_samples, region);
		regions_passed = regions_passed &&
			semantic_region.sampleCount() > 0u &&
			scaffold_region.sampleCount() > 0u;
		region_reports.emplace_back(
			regionIdentifier(region), semantic_region, scaffold_region);
	}
	const bool passed =
		directionPassed(semantic_to_scaffold, accepted_correspondence) &&
		directionPassed(scaffold_to_semantic, accepted_correspondence) &&
		regions_passed;
	return SemanticImplicitCorrespondenceReport(
		semantic_to_scaffold,
		scaffold_to_semantic,
		accepted_correspondence.percentile95DistanceTolerance(),
		accepted_correspondence.maximumDistanceTolerance(),
		accepted_correspondence.percentile95NormalAngleToleranceDegrees(),
		std::move(region_reports),
		passed);
}
