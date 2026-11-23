#include "geometry/service/FacialSkinMeshGenerator.h"
#include "geometry/service/FacialSkinSpecificationValidator.h"
#include "geometry/service/MeshTopologyAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
	if (!condition) throw std::runtime_error(message);
}

glm::dvec3 closest_triangle_point(const glm::dvec3 &point,
	                            const glm::dvec3 &first,
	                            const glm::dvec3 &second,
	                            const glm::dvec3 &third)
{
	const glm::dvec3 edge_first = second - first;
	const glm::dvec3 edge_second = third - first;
	const glm::dvec3 from_first = point - first;
	const double d1 = glm::dot(edge_first, from_first);
	const double d2 = glm::dot(edge_second, from_first);
	if (d1 <= 0.0 && d2 <= 0.0) return first;
	const glm::dvec3 from_second = point - second;
	const double d3 = glm::dot(edge_first, from_second);
	const double d4 = glm::dot(edge_second, from_second);
	if (d3 >= 0.0 && d4 <= d3) return second;
	const double third_region = d1 * d4 - d3 * d2;
	if (third_region <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
		return first + (d1 / (d1 - d3)) * edge_first;
	const glm::dvec3 from_third = point - third;
	const double d5 = glm::dot(edge_first, from_third);
	const double d6 = glm::dot(edge_second, from_third);
	if (d6 >= 0.0 && d5 <= d6) return third;
	const double second_region = d5 * d2 - d1 * d6;
	if (second_region <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
		return first + (d2 / (d2 - d6)) * edge_second;
	const double first_region = d3 * d6 - d5 * d4;
	if (first_region <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0)
		return second + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (third - second);
	const double inverse = 1.0 / (first_region + second_region + third_region);
	return first + second_region * inverse * edge_first + third_region * inverse * edge_second;
}

double verify_globe_clearance(const GeneratedPrimitiveMesh &generated,
	                         const FacialSkinLandmarks &landmarks)
{
	const Mesh &mesh = *generated.mesh();
	const glm::dvec3 centers[] = {glm::dvec3(landmarks.left_eyeball_center),
	                             glm::dvec3(landmarks.right_eyeball_center)};
	double minimum_clearance = std::numeric_limits<double>::infinity();
	for (const glm::ivec3 &face : mesh.faces) {
		const glm::dvec3 first(mesh.vertices[face.x]);
		const glm::dvec3 second(mesh.vertices[face.y]);
		const glm::dvec3 third(mesh.vertices[face.z]);
		for (const glm::dvec3 &center : centers) {
			const double clearance = glm::length(closest_triangle_point(center, first, second, third) - center)
				- landmarks.eyeball_radius;
			minimum_clearance = std::min(minimum_clearance, clearance);
		}
	}
	// This inspects complete facets, including back rims. A vertex-only check
	// cannot detect a triangle chord cutting through the curved globe.
	require(minimum_clearance >= -landmarks.eyeball_radius * 5.0e-6,
	        "skin triangle intersects an eyeball");
	return minimum_clearance;
}

int component_count(const std::vector<std::vector<int>> &adjacency,
	                bool ignore_isolated = false)
{
	std::vector<bool> visited(adjacency.size(), false);
	int components = 0;
	for (std::size_t first = 0; first < adjacency.size(); ++first) {
		if (visited[first] || (ignore_isolated && adjacency[first].empty())) continue;
		++components;
		std::queue<int> pending;
		pending.push(static_cast<int>(first));
		visited[first] = true;
		while (!pending.empty()) {
			const int vertex = pending.front();
			pending.pop();
			for (int neighbor : adjacency[vertex]) {
				if (visited[neighbor]) continue;
				visited[neighbor] = true;
				pending.push(neighbor);
			}
		}
	}
	return components;
}

void verify_geometry(const GeneratedPrimitiveMesh &generated, float characteristic_length = 0.16f)
{
	const Mesh &mesh = *generated.mesh();
	require(!mesh.faces.empty(), "face contains no triangles");
	require(mesh.normals.size() == mesh.vertices.size(), "missing per-vertex normals");
	require(mesh.texcoords.size() == mesh.vertices.size(), "missing rest-space texture coordinates");
	require(generated.faceSurfaceTags().size() == mesh.faces.size(), "missing surface tags");
	for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
		const glm::vec3 &vertex = mesh.vertices[index];
		require(std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z), "nonfinite vertex");
		require(std::fabs(glm::length(mesh.normals[index]) - 1.0f) < 2.0e-5f, "non-unit deformed normal");
	}
	std::map<std::pair<int, int>, int> edge_counts;
	std::vector<std::vector<int>> adjacency(mesh.vertices.size());
	for (std::size_t index = 0; index < mesh.faces.size(); ++index) {
		const glm::ivec3 &face = mesh.faces[index];
		for (int vertex : {face.x, face.y, face.z}) {
			require(vertex >= 0 && static_cast<std::size_t>(vertex) < mesh.vertices.size(), "invalid face index");
		}
		const glm::vec3 cross = glm::cross(mesh.vertices[face.y] - mesh.vertices[face.x],
		                                  mesh.vertices[face.z] - mesh.vertices[face.x]);
		require(glm::length(cross) > 1.0e-11f * characteristic_length * characteristic_length, "collapsed triangle");
		const glm::vec3 mean_normal = mesh.normals[face.x] + mesh.normals[face.y] + mesh.normals[face.z];
		require(glm::dot(glm::normalize(cross), glm::normalize(mean_normal)) > 0.0f,
		        "interpolated shading normal points behind its triangle at face " + std::to_string(index));
		if (generated.faceSurfaceTags()[index].boundaryIndex() == 0) {
			if (cross.z <= 1.0e-12f * characteristic_length * characteristic_length) {
				const auto &uv = mesh.texcoords[face.x];
				throw std::runtime_error("front-skin triangle flipped in its material chart at face " +
					std::to_string(index) + " uv=" + std::to_string(uv.x) + "," + std::to_string(uv.y) +
					" signed area=" + std::to_string(cross.z));
			}
		}
		for (const auto edge : {std::make_pair(face.x, face.y), std::make_pair(face.y, face.z),
		                         std::make_pair(face.z, face.x)}) {
			edge_counts[std::minmax(edge.first, edge.second)] += 1;
			adjacency[edge.first].push_back(edge.second);
			adjacency[edge.second].push_back(edge.first);
		}
	}
	require(component_count(adjacency) == 1, "skin and aperture margins must be one connected surface");
	std::vector<std::vector<int>> boundary_adjacency(mesh.vertices.size());
	for (const auto &edge : edge_counts) {
		require(edge.second <= 2, "nonmanifold edge");
		if (edge.second != 1) continue;
		boundary_adjacency[edge.first.first].push_back(edge.first.second);
		boundary_adjacency[edge.first.second].push_back(edge.first.first);
	}
	for (const auto &neighbors : boundary_adjacency) {
		require(neighbors.empty() || neighbors.size() == 2, "boundary is not a closed loop");
	}
	require(component_count(boundary_adjacency, true) == 4,
	        "expected outline plus exactly two eye apertures and one mouth aperture");
	const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(mesh, generated.faceSurfaceTags(), characteristic_length);
	require(topology.degenerate_triangle_count == 0, "native topology analyzer rejected triangles");
	require(topology.nonmanifold_edge_count == 0, "native topology analyzer found nonmanifold edges");
	require(!topology.isWatertight(), "front patch must not claim a closed volume");
}

std::shared_ptr<const FacialSkinShapeSpecification> validated(
	const FacialSkinShapeSpecificationCandidate &candidate)
{
	std::string diagnostic;
	auto specification = FacialSkinSpecificationValidator().validate(candidate, &diagnostic);
	require(static_cast<bool>(specification), diagnostic);
	return specification;
}

bool same_vertices(const Mesh &first, const Mesh &second)
{
	if (first.vertices.size() != second.vertices.size()) return false;
	for (std::size_t index = 0; index < first.vertices.size(); ++index) {
		if (first.vertices[index] != second.vertices[index]) return false;
	}
	return true;
}

void verify_invalid_input()
{
	FacialSkinSpecificationValidator validator;
	std::string diagnostic;
	FacialSkinShapeSpecificationCandidate candidate;
	candidate.controls.jaw_open = -0.01f;
	require(!validator.validate(candidate, &diagnostic), "negative control accepted");
	candidate.controls.jaw_open = 1.01f;
	require(!validator.validate(candidate, &diagnostic), "control above one accepted");
	candidate.controls.jaw_open = std::numeric_limits<float>::quiet_NaN();
	require(!validator.validate(candidate, &diagnostic), "NaN control accepted");
	candidate = {};
	candidate.horizontal_segments = 49;
	require(!validator.validate(candidate, &diagnostic), "unstable aperture tessellation accepted");
	candidate = {};
	candidate.vertical_segments = 1000000000;
	require(!validator.validate(candidate, &diagnostic), "unbounded tessellation accepted");
	candidate = {};
	candidate.size.x = 0.0f;
	require(!validator.validate(candidate, &diagnostic), "zero size accepted");
}

} // namespace

int main()
{
	try {
		FacialSkinMeshGenerator generator;
		FacialSkinShapeSpecificationCandidate candidate;
		const auto neutral_specification = validated(candidate);
		const auto neutral = generator.generate(*neutral_specification, nullptr);
		verify_geometry(neutral);
		const auto neutral_landmarks = generator.evaluateLandmarks(*neutral_specification);
		double minimum_globe_clearance = std::numeric_limits<double>::infinity();
		require(glm::length(glm::vec3(-neutral_landmarks.left_eye_outer_corner.x,
			neutral_landmarks.left_eye_outer_corner.y, neutral_landmarks.left_eye_outer_corner.z) -
			neutral_landmarks.right_eye_outer_corner) < 1.0e-7f, "neutral eye symmetry");
		require(glm::length(glm::vec3(-neutral_landmarks.mouth_left_corner.x,
			neutral_landmarks.mouth_left_corner.y, neutral_landmarks.mouth_left_corner.z) -
			neutral_landmarks.mouth_right_corner) < 1.0e-7f, "neutral mouth symmetry");
		for (unsigned pose = 0; pose < 128; ++pose) {
			candidate.controls = {
				static_cast<float>((pose >> 0) & 1), static_cast<float>((pose >> 1) & 1),
				static_cast<float>((pose >> 2) & 1), static_cast<float>((pose >> 3) & 1),
				static_cast<float>((pose >> 4) & 1), static_cast<float>((pose >> 5) & 1),
				static_cast<float>((pose >> 6) & 1)};
			const auto specification = validated(candidate);
			const auto generated = generator.generate(*specification, nullptr);
			try { verify_geometry(generated); }
			catch (const std::exception &error) {
				throw std::runtime_error("pose " + std::to_string(pose) + ": " + error.what());
			}
			minimum_globe_clearance = std::min(minimum_globe_clearance,
				verify_globe_clearance(generated, generator.evaluateLandmarks(*specification)));
			require(generated.mesh()->faces == neutral.mesh()->faces, "expression changed triangle topology");
			require(generated.mesh()->vertices.size() == neutral.mesh()->vertices.size(), "expression changed vertex count");
		}
		candidate = {};
		candidate.controls.blink_left = 1.0f;
		auto landmarks = generator.evaluateLandmarks(*validated(candidate));
		const float open_eye_gap = neutral_landmarks.left_eye_upper_margin.y - neutral_landmarks.left_eye_lower_margin.y;
		const float closed_eye_gap = landmarks.left_eye_upper_margin.y - landmarks.left_eye_lower_margin.y;
		require(closed_eye_gap > 0.0f && closed_eye_gap < 0.02f * open_eye_gap, "blink clearance/seal");
		require(landmarks.right_eye_upper_margin == neutral_landmarks.right_eye_upper_margin,
		        "left blink moved right eyelid");
		candidate = {};
		candidate.controls.jaw_open = 1.0f;
		landmarks = generator.evaluateLandmarks(*validated(candidate));
		require(landmarks.mouth_upper_margin.y - landmarks.mouth_lower_margin.y > 0.01f,
		        "jaw opening did not create a visible mouth aperture");
		candidate = {};
		candidate.controls.smile_left = 1.0f;
		landmarks = generator.evaluateLandmarks(*validated(candidate));
		require(landmarks.mouth_left_corner.y > neutral_landmarks.mouth_left_corner.y + 0.002f,
		        "smile did not raise the mouth corner");
		require(landmarks.mouth_left_corner.x > neutral_landmarks.mouth_left_corner.x,
		        "smile did not draw the mouth corner outwards");
		const auto neutral_again = generator.generate(*neutral_specification, nullptr);
		require(same_vertices(*neutral.mesh(), *neutral_again.mesh()), "return to neutral is not deterministic");
		candidate = {};
		candidate.horizontal_segments = 96;
		candidate.vertical_segments = 128;
		candidate.controls = {1, 1, 1, 1, 1, 1, 1};
		verify_geometry(generator.generate(*validated(candidate), nullptr));
		candidate.horizontal_segments = 192;
		candidate.vertical_segments = 256;
		const auto maximum_detail = validated(candidate);
		const auto maximum_geometry = generator.generate(*maximum_detail, nullptr);
		verify_geometry(maximum_geometry);
		verify_globe_clearance(maximum_geometry, generator.evaluateLandmarks(*maximum_detail));
		const auto default_report = MeshTopologyAnalyzer().analyze(*maximum_geometry.mesh(), maximum_geometry.faceSurfaceTags());
		const auto relative_report = MeshTopologyAnalyzer().analyze(*maximum_geometry.mesh(), maximum_geometry.faceSurfaceTags(), candidate.size.x);
		float world_volume = 0.0f;
		for (const glm::ivec3 &face : maximum_geometry.mesh()->faces) {
			const auto &vertices = maximum_geometry.mesh()->vertices;
			world_volume += glm::dot(vertices[face.x], glm::cross(vertices[face.y], vertices[face.z])) / 6.0f;
		}
		require(world_volume == relative_report.signed_volume,
		        "relative topology tolerance rescaled the world-unit volume");
		require(default_report.topology_hash == relative_report.topology_hash,
		        "relative topology tolerance changed the geometry hash");
		candidate.size *= 0.1f;
		const auto miniature_specification = validated(candidate);
		const auto miniature = generator.generate(*miniature_specification, nullptr);
		verify_geometry(miniature, candidate.size.x);
		verify_globe_clearance(miniature, generator.evaluateLandmarks(*miniature_specification));
		verify_invalid_input();
        // Zero identity explicitly retains every legacy vertex, canonical key and landmark.
        candidate = {};
        const auto old_zero = validated(candidate);
        candidate.identity_shape.coefficients.fill(0.0f);
        const auto explicit_zero = validated(candidate);
        require(old_zero->canonicalText() == explicit_zero->canonicalText(), "explicit zero identity changed canonical text");
        require(old_zero->key() == explicit_zero->key(), "explicit zero identity changed cache key");
        require(same_vertices(*neutral.mesh(), *generator.generate(*explicit_zero, nullptr).mesh()),
                "explicit zero identity changed historical positions");
        candidate.identity_shape.coefficients[0] = -.055f;
        candidate.identity_shape.coefficients[3] = .02f;
        candidate.identity_shape.coefficients[18] = .035f;
        candidate.identity_shape.coefficients[21] = .025f;
        const auto id_spec = validated(candidate);
        const auto id_mesh = generator.generate(*id_spec, nullptr);
        verify_geometry(id_mesh);
        require(!same_vertices(*neutral.mesh(), *id_mesh.mesh()), "nonzero identity changed no vertices");
        require(same_vertices(*id_mesh.mesh(), *generator.generate(*id_spec, nullptr).mesh()),
                "identity geometry was not deterministic");
        verify_globe_clearance(id_mesh, generator.evaluateLandmarks(*id_spec));
        candidate.identity_shape.coefficients[0] = std::numeric_limits<float>::quiet_NaN();
        std::string identity_diagnostic;
        require(!FacialSkinSpecificationValidator().validate(candidate, &identity_diagnostic),
                "nonfinite identity coefficient accepted");
        candidate.identity_shape.coefficients[0] = .9f;
        require(!FacialSkinSpecificationValidator().validate(candidate, &identity_diagnostic),
                "out-of-range identity coefficient accepted");
		std::cout << "FacialSkin geometry PASS: 128 extreme-control combinations, 1 connected skin, "
		          << "4 boundary loops, stable topology, normals, landmarks, input bounds, deterministic replay.\n"
		          << "vertices=" << neutral.mesh()->vertices.size() << " triangles=" << neutral.mesh()->faces.size()
		          << " neutral_eye_gap_m=" << open_eye_gap << " closed_eye_gap_m=" << closed_eye_gap
		          << " eyeball_radius_m=" << neutral_landmarks.eyeball_radius
		          << " minimum_triangle_globe_clearance_m=" << minimum_globe_clearance << '\n';
		return EXIT_SUCCESS;
	} catch (const std::exception &error) {
		std::cerr << "FacialSkin geometry FAIL: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
