#include "geometry/service/FacialElasticModelAssembler.h"

#include "geometry/service/FacialElasticSolver.h"
#include "geometry/service/FacialBendingStencilAssembler.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace {

double compactPatchWeight(const glm::dvec2 &uv, const glm::vec2 &center,
	                      const glm::vec2 &radii)
{
	const glm::dvec2 offset = (uv - glm::dvec2(center)) / glm::dvec2(radii);
	const double radius_squared = glm::dot(offset, offset);
	if (radius_squared >= 1.0) return 0.0;
	const double remaining = 1.0 - radius_squared;
	return remaining * remaining;
}

std::pair<double, double> materialScales(
	const glm::dvec2 &uv, const std::vector<FacialTissueRegion> &regions)
{
	double total_weight = 0.0;
	double young_sum = 0.0;
	double support_sum = 0.0;
	for (const auto &region : regions) {
		const double weight = compactPatchWeight(uv, region.center_uv, region.influence_radius_uv);
		total_weight += weight;
		young_sum += weight * region.young_modulus_scale;
		support_sum += weight * region.support_stiffness_scale;
	}
	// A partition of unity gives an isolated patch its authored centre value,
	// blends overlaps, and never multiplies overlapping scales without bound.
	const double base_weight = std::max(0.0, 1.0 - total_weight);
	const double divisor = std::max(1.0, total_weight);
	return {(base_weight + young_sum) / divisor,
	        (base_weight + support_sum) / divisor};
}

bool protectiveAttachment(const glm::dvec2 &uv)
{
	if (uv.x < 1.0e-7 || uv.x > 1.0 - 1.0e-7 ||
	    uv.y < 1.0e-7 || uv.y > 1.0 - 1.0e-7) return true;
	for (double center_u : {7.0 / 24.0, 17.0 / 24.0}) {
		const double horizontal = std::fabs((uv.x - center_u) / 0.125);
		const double vertical = std::fabs((uv.y - 21.0 / 32.0) / 0.0625);
		if (std::max(horizontal, vertical) <= 1.75 + 1.0e-6) return true;
	}
	return false;
}

glm::dvec3 interpolateRestPoint(const Mesh &mesh,
	                          const std::vector<MeshSurfaceTag> &surface_tags,
	                          const glm::vec2 &uv, const std::string &label)
{
	for (std::size_t face_index = 0; face_index < mesh.faces.size(); ++face_index) {
		if (surface_tags[face_index].boundaryIndex() != 0) continue;
		const auto &face = mesh.faces[face_index];
		const glm::dvec2 first(mesh.texcoords[face.x]);
		const glm::dvec2 edge_one = glm::dvec2(mesh.texcoords[face.y]) - first;
		const glm::dvec2 edge_two = glm::dvec2(mesh.texcoords[face.z]) - first;
		const glm::dvec2 offset = glm::dvec2(uv) - first;
		const double determinant = edge_one.x * edge_two.y - edge_one.y * edge_two.x;
		if (std::fabs(determinant) < 1.0e-16) continue;
		const double second = (offset.x * edge_two.y - offset.y * edge_two.x) / determinant;
		const double third = (edge_one.x * offset.y - edge_one.y * offset.x) / determinant;
		const double first_weight = 1.0 - second - third;
		if (first_weight < -1.0e-5 || second < -1.0e-5 || third < -1.0e-5) continue;
		return first_weight * glm::dvec3(mesh.vertices[face.x]) +
		       second * glm::dvec3(mesh.vertices[face.y]) + third * glm::dvec3(mesh.vertices[face.z]);
	}
	throw std::runtime_error(label + " must lie on the neutral skin chart, not inside an aperture.");
}

std::vector<FacialElasticInsertionWeight> normalizedPatch(
	const Mesh &mesh, const std::vector<double> &areas,
	const std::vector<std::vector<std::size_t>> &adjacency,
	const glm::vec2 &center, const glm::vec2 &radius, const std::string &label)
{
	std::vector<double> weights(mesh.vertices.size(), 0.0);
	std::size_t seed = mesh.vertices.size();
	double nearest = std::numeric_limits<double>::infinity();
	for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
		const glm::dvec2 uv(mesh.texcoords[index]);
		weights[index] = areas[index] * compactPatchWeight(uv, center, radius);
		if (weights[index] <= 0.0) continue;
		const glm::dvec2 offset = uv - glm::dvec2(center);
		const double distance = glm::dot(offset, offset);
		if (distance < nearest) { nearest = distance; seed = index; }
	}
	if (seed == mesh.vertices.size()) throw std::runtime_error(label + " has no skin vertices in its support.");
	// A compact chart patch may touch disconnected banks of an aperture. Keep
	// the component containing its nearest material node, never bridge the hole.
	std::vector<bool> visited(mesh.vertices.size(), false);
	std::queue<std::size_t> pending;
	std::vector<FacialElasticInsertionWeight> patch;
	visited[seed] = true;
	pending.push(seed);
	double total = 0.0;
	while (!pending.empty()) {
		const std::size_t vertex = pending.front(); pending.pop();
		patch.push_back({vertex, weights[vertex]});
		total += weights[vertex];
		for (std::size_t neighbor : adjacency[vertex]) {
			if (visited[neighbor] || weights[neighbor] <= 0.0) continue;
			visited[neighbor] = true; pending.push(neighbor);
		}
	}
	if (!(total > 0.0) || !std::isfinite(total)) throw std::runtime_error(label + " has zero material area.");
	for (auto &insertion : patch) insertion.weight /= total;
	return patch;
}

} // namespace

FacialSkinMechanicalState FacialElasticModelAssembler::assemble(
	const FacialSkinShapeSpecification &specification, const Mesh &neutral_mesh,
	const std::vector<MeshSurfaceTag> &surface_tags) const
{
	if (neutral_mesh.vertices.size() != neutral_mesh.texcoords.size() ||
	    neutral_mesh.faces.size() != surface_tags.size()) {
		throw std::runtime_error("Facial elastic assembly requires native rest UVs and surface tags.");
	}
	FacialSkinMechanicalState state;
	auto &system = state.system;
	const std::size_t count = neutral_mesh.vertices.size();
	system.rest_positions.assign(neutral_mesh.vertices.begin(), neutral_mesh.vertices.end());
	system.support_targets = system.rest_positions;
	system.fixed_vertices.resize(count, false);
	system.support_stiffness_n_per_m.resize(count, 0.0);
	state.rest_vertex_areas_m2.resize(count, 0.0);
	state.young_modulus_scales.resize(count, 1.0);
	state.support_stiffness_scales.resize(count, 1.0);
	std::vector<std::vector<std::size_t>> adjacency(count);
	const auto &material = specification.elasticMaterial();
	for (const auto &face : neutral_mesh.faces) {
		const std::array<std::size_t, 3> indices{{static_cast<std::size_t>(face.x),
			static_cast<std::size_t>(face.y), static_cast<std::size_t>(face.z)}};
		const glm::dvec2 uv = (glm::dvec2(neutral_mesh.texcoords[face.x]) +
			glm::dvec2(neutral_mesh.texcoords[face.y]) + glm::dvec2(neutral_mesh.texcoords[face.z])) / 3.0;
		const auto scales = materialScales(uv, specification.tissueRegions());
		auto triangle = FacialElasticSolver::makeRestTriangle(indices, system.rest_positions,
			material.young_modulus_pa * scales.first, material.poisson_ratio, material.thickness_m);
		for (std::size_t vertex : indices) state.rest_vertex_areas_m2[vertex] += triangle.rest_area_m2 / 3.0;
		for (int corner = 0; corner < 3; ++corner) {
			const std::size_t first = indices[corner], second = indices[(corner + 1) % 3];
			adjacency[first].push_back(second); adjacency[second].push_back(first);
		}
		system.triangles.push_back(triangle);
	}
	for (std::size_t index = 0; index < count; ++index) {
		const glm::dvec2 uv(neutral_mesh.texcoords[index]);
		const auto scales = materialScales(uv, specification.tissueRegions());
		state.young_modulus_scales[index] = scales.first;
		state.support_stiffness_scales[index] = scales.second;
		system.fixed_vertices[index] = protectiveAttachment(uv);
		system.support_stiffness_n_per_m[index] = material.support_stiffness_n_per_m3 *
			state.rest_vertex_areas_m2[index] * scales.second;
	}
	for (const auto &attachment : specification.attachments()) {
		const auto patch = normalizedPatch(neutral_mesh, state.rest_vertex_areas_m2, adjacency,
			attachment.center_uv, attachment.influence_radius_uv, "Attachment '" + attachment.name + "'");
		for (const auto &weight : patch) {
			system.support_stiffness_n_per_m[weight.vertex_index] += attachment.stiffness_n_per_m * weight.weight;
		}
	}
	for (const auto &definition : specification.muscles()) {
		FacialElasticMuscle muscle;
		muscle.name = definition.name;
		muscle.origin = interpolateRestPoint(neutral_mesh, surface_tags, definition.origin_uv,
			"Muscle '" + definition.name + "' origin");
		interpolateRestPoint(neutral_mesh, surface_tags, definition.insertion_uv,
			"Muscle '" + definition.name + "' insertion");
		muscle.insertions = normalizedPatch(neutral_mesh, state.rest_vertex_areas_m2, adjacency,
			definition.insertion_uv, definition.influence_radius_uv, "Muscle '" + definition.name + "'");
		glm::dvec3 insertion(0.0);
		double free_weight = 0.0;
		for (const auto &weight : muscle.insertions) {
			insertion += weight.weight * system.rest_positions[weight.vertex_index];
			if (!system.fixed_vertices[weight.vertex_index]) free_weight += weight.weight;
		}
		if (free_weight < 1.0e-8) throw std::runtime_error("Muscle '" + definition.name + "' insertion is entirely fixed.");
		muscle.rest_length_m = glm::length(insertion - muscle.origin);
		if (muscle.rest_length_m <= specification.size().x * 1.0e-6)
			throw std::runtime_error("Muscle '" + definition.name + "' has zero neutral cable length.");
		muscle.activation = definition.activation;
		muscle.maximum_shortening = definition.maximum_shortening;
		muscle.stiffness_n_per_m = definition.stiffness_n_per_m;
		system.muscles.push_back(std::move(muscle));
	}
	// Leave the original membrane-only path untouched, including meshes whose
	// sharp neutral folds are unsuitable for a curvature reconstruction.
	if (specification.bendingSettings().stiffness_scale > 0.0f) {
		state.bending_assembly = FacialBendingStencilAssembler().assemble(
			system, specification.bendingSettings().stiffness_scale);
	}
	return state;
}
