#include "geometry/model/FacialSkinMesh.h"
#include "geometry/service/FacialSkinAdmissibility.h"
#include "geometry/service/FacialSkinMeshGenerator.h"
#include "geometry/service/FacialSkinSpecificationValidator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class FacialShellTestAssertions
{
public:
	static void require(bool condition, const std::string &message)
	{
		if (!condition) throw std::runtime_error(message);
	}
};

class FacialTriangleQualityStatistics
{
public:
	void add(const glm::dvec3 &first, const glm::dvec3 &second, const glm::dvec3 &third)
	{
		const std::array<glm::dvec3, 3> points{{first, second, third}};
		const double double_area = glm::length(glm::cross(second - first, third - first));
		double longest_squared = 0.0, sum_squared = 0.0;
		for (int corner = 0; corner < 3; ++corner) {
			const glm::dvec3 edge = points[(corner + 1) % 3] - points[corner];
			const double length_squared = glm::dot(edge, edge);
			longest_squared = std::max(longest_squared, length_squared);
			sum_squared += length_squared;
			const glm::dvec3 other = points[(corner + 2) % 3] - points[corner];
			minimum_angle_degrees = std::min(minimum_angle_degrees,
				std::atan2(glm::length(glm::cross(edge, other)), glm::dot(edge, other)) * 180.0 / pi_);
		}
		FacialShellTestAssertions::require(double_area > 0.0 && sum_squared > 0.0, "degenerate quality measurement triangle");
		aspects.push_back(longest_squared / double_area);
		minimum_mean_ratio = std::min(minimum_mean_ratio, 2.0 * std::sqrt(3.0) * double_area / sum_squared);
		minimum_area_m2 = std::min(minimum_area_m2, 0.5 * double_area);
	}

	double maximumAspect() const
	{
		return aspects.empty() ? 0.0 : *std::max_element(aspects.begin(), aspects.end());
	}

	double percentileAspect(double fraction) const
	{
		if (aspects.empty()) return 0.0;
		auto sorted = aspects;
		std::sort(sorted.begin(), sorted.end());
		return sorted[static_cast<std::size_t>(fraction * (sorted.size() - 1))];
	}

	void report(const std::string &label) const
	{
		std::cout << "  " << label << " faces=" << aspects.size() << " max_aspect=" << maximumAspect()
			<< " p95_aspect=" << percentileAspect(0.95) << " min_angle_deg=" << minimum_angle_degrees
			<< " min_mean_ratio=" << minimum_mean_ratio << " min_area_m2=" << minimum_area_m2 << '\n';
	}

	std::vector<double> aspects;
	double minimum_angle_degrees = 180.0;
	double minimum_mean_ratio = 1.0;
	double minimum_area_m2 = std::numeric_limits<double>::infinity();

private:
	static constexpr double pi_ = 3.14159265358979323846;
};

class FacialMouthMaterialChart
{
public:
	static bool insideReplacement(const glm::vec2 &uv)
	{
		return uv.x > 10.0f / 48.0f && uv.x < 38.0f / 48.0f &&
			uv.y > 12.0f / 64.0f && uv.y < 28.0f / 64.0f;
	}

	static bool edgeCrossesAperture(const glm::vec2 &first, const glm::vec2 &second)
	{
		const glm::dvec2 minimum(14.0 / 48.0 + 1.0e-7, 16.0 / 64.0 + 1.0e-7);
		const glm::dvec2 maximum(34.0 / 48.0 - 1.0e-7, 24.0 / 64.0 - 1.0e-7);
		const glm::dvec2 origin(first), direction = glm::dvec2(second) - origin;
		double first_parameter = 0.0, last_parameter = 1.0;
		for (int axis = 0; axis < 2; ++axis) {
			if (std::fabs(direction[axis]) < 1.0e-15) {
				if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis]) return false;
				continue;
			}
			double entry = (minimum[axis] - origin[axis]) / direction[axis];
			double exit = (maximum[axis] - origin[axis]) / direction[axis];
			if (entry > exit) std::swap(entry, exit);
			first_parameter = std::max(first_parameter, entry);
			last_parameter = std::min(last_parameter, exit);
			if (first_parameter > last_parameter) return false;
		}
		return first_parameter <= last_parameter;
	}
};

class FacialShellGeometrySample
{
public:
	FacialShellGeometrySample(const std::string &sample_label, FacialMouthMeshMode mode, int detail,
		float bending_scale = 0.0f, float left_activation = 0.0f, float right_activation = 0.0f)
		: label(sample_label)
	{
		FacialSkinShapeSpecificationCandidate candidate;
		candidate.response_mode = FacialSkinResponseMode::Elastic;
		candidate.mouth_mesh_mode = mode;
		candidate.horizontal_segments = 48 * detail;
		candidate.vertical_segments = 64 * detail;
		candidate.elastic_solve_settings.maximum_iterations = 2000;
		candidate.bending_settings.stiffness_scale = bending_scale;
		if (left_activation != 0.0f || right_activation != 0.0f) {
			candidate.muscles = {
				{"left_smile", {0.87f, 0.56f}, {34.0f / 48.0f, 20.0f / 64.0f}, {0.055f, 0.08f}, left_activation, 0.04f, 40.0f},
				{"right_smile", {0.13f, 0.56f}, {14.0f / 48.0f, 20.0f / 64.0f}, {0.055f, 0.08f}, right_activation, 0.04f, 40.0f}};
		}
		std::string diagnostic;
		specification = FacialSkinSpecificationValidator().validate(candidate, &diagnostic);
		FacialShellTestAssertions::require(static_cast<bool>(specification), label + ": " + diagnostic);
		auto generated = FacialSkinMeshGenerator().generate(*specification, &diagnostic);
		face = std::dynamic_pointer_cast<FacialSkinMesh>(generated.mesh());
		FacialShellTestAssertions::require(face && !face->vertices.empty(), label + ": " + diagnostic);
		tags = generated.faceSurfaceTags();
		FacialShellTestAssertions::require(face->mechanical_state && face->mechanical_state->solve_result.converged,
			label + ": neutral mechanical state missing or unconverged");
		if (left_activation == 0.0f && right_activation == 0.0f) {
			FacialShellTestAssertions::require(face->mechanical_state->solve_result.iterations == 0 &&
				face->mechanical_state->solve_result.final_evaluation.total_energy_j < 1.0e-18,
				label + ": neutral mesh has nonzero elastic prestress");
		}
	}

	std::string label;
	std::shared_ptr<const FacialSkinShapeSpecification> specification;
	std::shared_ptr<FacialSkinMesh> face;
	std::vector<MeshSurfaceTag> tags;
	FacialTriangleQualityStatistics entire_surface;
	FacialTriangleQualityStatistics mouth_front;
	FacialTriangleQualityStatistics mouth_rim;
};

class FacialShellTopologyVerifier
{
public:
	bool verify(FacialShellGeometrySample &sample) const
	{
		const auto &mesh = *sample.face;
		FacialShellTestAssertions::require(mesh.normals.size() == mesh.vertices.size() &&
			mesh.texcoords.size() == mesh.vertices.size() && sample.tags.size() == mesh.faces.size(),
			sample.label + ": incomplete native mesh attributes");
		std::map<std::pair<int, int>, int> edge_counts;
		std::vector<std::vector<int>> adjacency(mesh.vertices.size());
		double minimum_normal_alignment = 1.0;
		std::size_t worst_normal_face = 0;
		for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
			const auto &point = mesh.vertices[index];
			FacialShellTestAssertions::require(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z),
				sample.label + ": nonfinite position");
			FacialShellTestAssertions::require(std::isfinite(glm::length(mesh.normals[index])) &&
				std::fabs(glm::length(mesh.normals[index]) - 1.0f) < 2.0e-5f, sample.label + ": invalid native normal");
		}
		for (std::size_t index = 0; index < mesh.faces.size(); ++index) {
			const auto &triangle = mesh.faces[index];
			for (int vertex : {triangle.x, triangle.y, triangle.z})
				FacialShellTestAssertions::require(vertex >= 0 && static_cast<std::size_t>(vertex) < mesh.vertices.size(),
					sample.label + ": invalid triangle index");
			const glm::dvec3 first(mesh.vertices[triangle.x]), second(mesh.vertices[triangle.y]), third(mesh.vertices[triangle.z]);
			const glm::dvec3 area_normal = glm::cross(second - first, third - first);
			FacialShellTestAssertions::require(glm::length(area_normal) > 2.56e-12, sample.label + ": collapsed triangle");
			const auto &rest = sample.face->mechanical_state->system.rest_positions;
			const glm::dvec3 rest_normal = glm::normalize(glm::cross(rest[triangle.y] - rest[triangle.x],
				rest[triangle.z] - rest[triangle.x]));
			FacialShellTestAssertions::require(glm::dot(area_normal, rest_normal) > 2.56e-12,
				sample.label + ": facet exceeds its neutral-normal hemisphere");
			const glm::dvec3 shading_normal = glm::dvec3(mesh.normals[triangle.x]) + glm::dvec3(mesh.normals[triangle.y]) +
				glm::dvec3(mesh.normals[triangle.z]);
			const double alignment = glm::dot(glm::normalize(area_normal), glm::normalize(shading_normal));
			if (alignment < minimum_normal_alignment) { minimum_normal_alignment = alignment; worst_normal_face = index; }
			if (sample.specification->mouthMeshMode() == FacialMouthMeshMode::Quality) {
				for (int vertex : {triangle.x, triangle.y, triangle.z})
					FacialShellTestAssertions::require(glm::dot(area_normal, glm::dvec3(mesh.normals[vertex])) > 0.0,
						sample.label + ": vertex normal is outside its common incident-facet hemisphere");
			}
			sample.entire_surface.add(first, second, third);
			if (sample.tags[index].boundaryIndex() == 3) sample.mouth_rim.add(first, second, third);
			if (sample.tags[index].boundaryIndex() == 0) {
				const glm::dvec2 uv_first(mesh.texcoords[triangle.x]), uv_second(mesh.texcoords[triangle.y]), uv_third(mesh.texcoords[triangle.z]);
				const glm::dvec2 edge_one = uv_second - uv_first, edge_two = uv_third - uv_first;
				FacialShellTestAssertions::require(edge_one.x * edge_two.y - edge_one.y * edge_two.x > 0.0,
					sample.label + ": inverted material UV triangle");
				if (FacialMouthMaterialChart::insideReplacement(glm::vec2((uv_first + uv_second + uv_third) / 3.0)))
					sample.mouth_front.add(first, second, third);
				for (int corner = 0; corner < 3; ++corner)
					FacialShellTestAssertions::require(!FacialMouthMaterialChart::edgeCrossesAperture(
						glm::vec2(mesh.texcoords[triangle[corner]]), glm::vec2(mesh.texcoords[triangle[(corner + 1) % 3]])),
						sample.label + ": front edge crosses the true material mouth aperture");
			}
			for (int corner = 0; corner < 3; ++corner) {
				const int first_index = triangle[corner], second_index = triangle[(corner + 1) % 3];
				++edge_counts[std::minmax(first_index, second_index)];
				adjacency[first_index].push_back(second_index);
				adjacency[second_index].push_back(first_index);
			}
		}
		FacialShellTestAssertions::require(componentCount(adjacency, false) == 1, sample.label + ": disconnected mesh");
		std::vector<std::vector<int>> boundary(mesh.vertices.size());
		for (const auto &entry : edge_counts) {
			FacialShellTestAssertions::require(entry.second <= 2, sample.label + ": nonmanifold edge");
			if (entry.second == 1) {
				boundary[entry.first.first].push_back(entry.first.second);
				boundary[entry.first.second].push_back(entry.first.first);
			}
		}
		for (const auto &neighbors : boundary)
			FacialShellTestAssertions::require(neighbors.empty() || neighbors.size() == 2, sample.label + ": broken boundary loop");
		FacialShellTestAssertions::require(componentCount(boundary, true) == 4, sample.label + ": aperture count changed");
		const std::vector<glm::dvec3> positions(mesh.vertices.begin(), mesh.vertices.end());
		FacialSkinAdmissibility guard(mesh, sample.tags, mesh.landmarks, std::vector<bool>(positions.size(), false), 0.16);
		FacialShellTestAssertions::require(guard.accepts(positions), sample.label + ": neutral geometric guard failed");
		const double clearance = guard.minimumGlobeClearance(positions);
		FacialShellTestAssertions::require(clearance >= 0.0, sample.label + ": a complete triangle intersects an eye globe");
		std::cout << sample.label << " vertices=" << mesh.vertices.size() << " triangles=" << mesh.faces.size()
			<< " boundaries=4 globe_clearance_m=" << clearance << " min_shading_alignment=" << minimum_normal_alignment
			<< " worst_shading_face=" << worst_normal_face << " iterations=" << mesh.mechanical_state->solve_result.iterations
			<< " relative_force=" << mesh.mechanical_state->solve_result.relative_free_force << '\n';
		sample.entire_surface.report("all");
		sample.mouth_front.report("mouth_front");
		sample.mouth_rim.report("mouth_rim");
		if (minimum_normal_alignment <= 0.0) {
			for (int vertex : {mesh.faces[worst_normal_face].x, mesh.faces[worst_normal_face].y, mesh.faces[worst_normal_face].z}) {
				const auto &point = mesh.vertices[vertex], &normal = mesh.normals[vertex], &uv = mesh.texcoords[vertex];
				std::cout << "    shading_vertex=" << vertex << " uv=" << uv.x << ',' << uv.y
					<< " position=" << point.x << ',' << point.y << ',' << point.z
					<< " normal=" << normal.x << ',' << normal.y << ',' << normal.z << '\n';
			}
		}
		return minimum_normal_alignment > 0.0;
	}

private:
	static int componentCount(const std::vector<std::vector<int>> &adjacency, bool ignore_isolated)
	{
		std::vector<bool> visited(adjacency.size(), false);
		int components = 0;
		for (std::size_t start = 0; start < adjacency.size(); ++start) {
			if (visited[start] || (ignore_isolated && adjacency[start].empty())) continue;
			++components;
			std::queue<int> pending;
			pending.push(static_cast<int>(start)); visited[start] = true;
			while (!pending.empty()) {
				const int current = pending.front(); pending.pop();
				for (int next : adjacency[current]) {
					if (visited[next]) continue;
					visited[next] = true; pending.push(next);
				}
			}
		}
		return components;
	}
};

class FacialMouthNeutralShapeComparison
{
public:
	void verifyUnchangedRegion(const FacialShellGeometrySample &legacy, const FacialShellGeometrySample &quality) const
	{
		const auto collect = [](const Mesh &mesh) {
			std::map<std::pair<float, float>, std::vector<glm::vec3>> by_uv;
			for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
				const glm::vec2 uv(mesh.texcoords[index]);
				if (!FacialMouthMaterialChart::insideReplacement(uv)) by_uv[{uv.x, uv.y}].push_back(mesh.vertices[index]);
			}
			for (auto &entry : by_uv)
				std::sort(entry.second.begin(), entry.second.end(), [](const glm::vec3 &first, const glm::vec3 &second) { return first.z < second.z; });
			return by_uv;
		};
		FacialShellTestAssertions::require(collect(*legacy.face) == collect(*quality.face),
			quality.label + ": geometry or material coordinates outside the mouth region changed");
		const auto &old = legacy.face->landmarks;
		const auto &current = quality.face->landmarks;
		FacialShellTestAssertions::require(old.mouth_left_corner == current.mouth_left_corner &&
			old.mouth_right_corner == current.mouth_right_corner && old.mouth_upper_margin == current.mouth_upper_margin &&
			old.mouth_lower_margin == current.mouth_lower_margin && old.left_eyeball_center == current.left_eyeball_center &&
			old.right_eyeball_center == current.right_eyeball_center && old.eyeball_radius == current.eyeball_radius,
			quality.label + ": a semantic mouth landmark or eye reference changed");
	}

	void reportMaterialSurfaceDeviation(const FacialShellGeometrySample &first, const FacialShellGeometrySample &second) const
	{
		double maximum = 0.0, sum_squared = 0.0;
		std::size_t count = 0;
		glm::dvec2 worst_uv(0.0);
		glm::dvec3 worst_difference(0.0);
		std::string worst_direction;
		for (const auto &direction : {std::make_pair(&first, &second), std::make_pair(&second, &first)}) {
			const auto &sample = *direction.first;
			for (std::size_t index = 0; index < sample.face->faces.size(); ++index) {
				if (sample.tags[index].boundaryIndex() != 0) continue;
				const auto &face = sample.face->faces[index];
				const glm::dvec2 uv = (glm::dvec2(sample.face->texcoords[face.x]) +
					glm::dvec2(sample.face->texcoords[face.y]) + glm::dvec2(sample.face->texcoords[face.z])) / 3.0;
				if (!FacialMouthMaterialChart::insideReplacement(glm::vec2(uv))) continue;
				const glm::dvec3 point = (glm::dvec3(sample.face->vertices[face.x]) +
					glm::dvec3(sample.face->vertices[face.y]) + glm::dvec3(sample.face->vertices[face.z])) / 3.0;
				const glm::dvec3 offset = point - interpolateMaterialPoint(*direction.second, uv);
				const double difference = glm::length(offset);
				if (difference > maximum) {
					maximum = difference; worst_uv = uv; worst_difference = offset;
					worst_direction = direction.first->label;
				}
				sum_squared += difference * difference; ++count;
			}
		}
		std::cout << "  " << first.label << " versus " << second.label << " same_UV_PL_max_m=" << maximum
			<< " same_UV_PL_rms_m=" << std::sqrt(sum_squared / count) << " samples=" << count
			<< " worst_uv=" << worst_uv.x << ',' << worst_uv.y << " direction=" << worst_direction
			<< " difference_xyz_m=" << worst_difference.x << ',' << worst_difference.y << ',' << worst_difference.z << '\n';
	}

private:
	static glm::dvec3 interpolateMaterialPoint(const FacialShellGeometrySample &sample, const glm::dvec2 &uv)
	{
		for (std::size_t index = 0; index < sample.face->faces.size(); ++index) {
			if (sample.tags[index].boundaryIndex() != 0) continue;
			const auto &face = sample.face->faces[index];
			const glm::dvec2 first(sample.face->texcoords[face.x]);
			const glm::dvec2 edge_one = glm::dvec2(sample.face->texcoords[face.y]) - first;
			const glm::dvec2 edge_two = glm::dvec2(sample.face->texcoords[face.z]) - first;
			const glm::dvec2 offset = uv - first;
			const double determinant = edge_one.x * edge_two.y - edge_one.y * edge_two.x;
			const double second = (offset.x * edge_two.y - offset.y * edge_two.x) / determinant;
			const double third = (edge_one.x * offset.y - edge_one.y * offset.x) / determinant;
			const double first_weight = 1.0 - second - third;
			if (first_weight < -1.0e-7 || second < -1.0e-7 || third < -1.0e-7) continue;
			return first_weight * glm::dvec3(sample.face->vertices[face.x]) +
				second * glm::dvec3(sample.face->vertices[face.y]) + third * glm::dvec3(sample.face->vertices[face.z]);
		}
		throw std::runtime_error(sample.label + ": material chart has a coverage gap");
	}
};

class FacialShellConfigurationVerifier
{
public:
	void verify() const
	{
		FacialSkinSpecificationValidator validator;
		std::string diagnostic;
		FacialSkinShapeSpecificationCandidate candidate;
		candidate.mouth_mesh_mode = FacialMouthMeshMode::Quality;
		FacialShellTestAssertions::require(!validator.validate(candidate, &diagnostic), "analytic quality mouth mesh accepted");
		candidate = {};
		candidate.has_mouth_mesh_option = true;
		FacialShellTestAssertions::require(!validator.validate(candidate, &diagnostic), "analytic explicit legacy mouth mesh accepted");
		candidate = {};
		candidate.response_mode = FacialSkinResponseMode::Elastic;
		const auto omitted = validator.validate(candidate, &diagnostic);
		candidate.has_mouth_mesh_option = true;
		candidate.has_bending_option = true;
		const auto explicit_zero = validator.validate(candidate, &diagnostic);
		FacialShellTestAssertions::require(omitted && explicit_zero && omitted->key() == explicit_zero->key(),
			"explicit legacy/zero changed the membrane v2 key");
		candidate.mouth_mesh_mode = static_cast<FacialMouthMeshMode>(37);
		FacialShellTestAssertions::require(!validator.validate(candidate, &diagnostic), "invalid mouth mode accepted");
		candidate.mouth_mesh_mode = FacialMouthMeshMode::Quality;
		candidate.bending_settings.stiffness_scale = -0.1f;
		FacialShellTestAssertions::require(!validator.validate(candidate, &diagnostic), "negative bending accepted");
		candidate.bending_settings.stiffness_scale = std::numeric_limits<float>::quiet_NaN();
		FacialShellTestAssertions::require(!validator.validate(candidate, &diagnostic), "nonfinite bending accepted");
	}
};

} // namespace

int main()
{
	try {
		std::cout << std::setprecision(9);
		FacialShellConfigurationVerifier().verify();
		FacialShellGeometrySample legacy_coarse("legacy_48x64", FacialMouthMeshMode::Legacy, 1);
		FacialShellGeometrySample quality_coarse("quality_48x64", FacialMouthMeshMode::Quality, 1);
		FacialShellGeometrySample legacy_fine("legacy_96x128", FacialMouthMeshMode::Legacy, 2);
		FacialShellGeometrySample quality_fine("quality_96x128", FacialMouthMeshMode::Quality, 2);
		FacialShellTopologyVerifier topology;
		bool normals_valid = true;
		for (auto *sample : {&legacy_coarse, &quality_coarse, &legacy_fine, &quality_fine}) normals_valid &= topology.verify(*sample);
		FacialMouthNeutralShapeComparison shapes;
		shapes.verifyUnchangedRegion(legacy_coarse, quality_coarse);
		shapes.verifyUnchangedRegion(legacy_fine, quality_fine);
		shapes.reportMaterialSurfaceDeviation(legacy_coarse, quality_coarse);
		shapes.reportMaterialSurfaceDeviation(legacy_fine, quality_fine);
		shapes.reportMaterialSurfaceDeviation(quality_coarse, quality_fine);
		FacialShellTestAssertions::require(normals_valid, "a quality shading normal opposes its native facet");
		FacialShellTestAssertions::require(quality_coarse.mouth_front.maximumAspect() < legacy_coarse.mouth_front.maximumAspect() &&
			quality_fine.mouth_front.maximumAspect() < legacy_fine.mouth_front.maximumAspect(),
			"quality annulus did not improve the worst mouth-front aspect ratio");
		const FacialShellGeometrySample repeated("quality_repeat", FacialMouthMeshMode::Quality, 1);
		FacialShellTestAssertions::require(repeated.face->vertices == quality_coarse.face->vertices &&
			repeated.face->texcoords == quality_coarse.face->texcoords && repeated.face->faces == quality_coarse.face->faces &&
			repeated.face->normals == quality_coarse.face->normals, "quality mesh depends on prior detail queries");
		for (int bending : {0, 1}) {
			for (int side : {0, 1}) {
				FacialShellGeometrySample active("quality_bending" + std::to_string(bending) + (side == 0 ? "_left" : "_right"),
					FacialMouthMeshMode::Quality, 1, static_cast<float>(bending), side == 0 ? 0.5f : 0.0f, side == 1 ? 0.5f : 0.0f);
				FacialShellTestAssertions::require(topology.verify(active), "deformed quality shading normal opposes a facet");
				FacialShellTestAssertions::require(active.face->faces == quality_coarse.face->faces &&
					active.face->texcoords == quality_coarse.face->texcoords, "activation or bending changed quality material topology");
			}
		}
		std::cout << "Facial shell geometry PASS: neutral quality, true material aperture, unchanged outside geometry/landmarks, "
			<< "four boundaries, complete globe clearance, normals and exact repeat verified.\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Facial shell geometry FAIL: " << error.what() << '\n';
		return 1;
	}
}
