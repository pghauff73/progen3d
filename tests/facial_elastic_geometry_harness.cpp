#include "geometry/service/FacialSkinMeshGenerator.h"
#include "geometry/service/FacialSkinSpecificationValidator.h"
#include "geometry/service/FacialElasticSolver.h"
#include "geometry/service/FacialSkinAdmissibility.h"
#include "geometry/model/FacialSkinMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class FacialElasticTestAssertions
{
public:
	static void require(bool condition, const std::string &message)
	{
		if (!condition) throw std::runtime_error(message);
	}
};

class FacialTriangleGlobeClearanceEvaluator
{
public:
	double evaluate(const Mesh &mesh, const FacialSkinLandmarks &landmarks) const
	{
		const std::array<glm::dvec3, 2> centers = {{
			glm::dvec3(landmarks.left_eyeball_center),
			glm::dvec3(landmarks.right_eyeball_center)}};
		double minimum_clearance = std::numeric_limits<double>::infinity();
		for (const glm::ivec3 &face : mesh.faces) {
			const glm::dvec3 first(mesh.vertices[face.x]);
			const glm::dvec3 second(mesh.vertices[face.y]);
			const glm::dvec3 third(mesh.vertices[face.z]);
			for (const glm::dvec3 &center : centers) {
				const glm::dvec3 closest = closestPoint(center, first, second, third);
				minimum_clearance = std::min(minimum_clearance,
					glm::length(closest - center) - landmarks.eyeball_radius);
			}
		}
		return minimum_clearance;
	}

private:
	// A complete facet test is necessary: vertices outside a sphere do not
	// prevent the intervening triangle from cutting through its surface.
	static glm::dvec3 closestPoint(const glm::dvec3 &point,
		const glm::dvec3 &first, const glm::dvec3 &second, const glm::dvec3 &third)
	{
		const glm::dvec3 first_edge = second - first;
		const glm::dvec3 second_edge = third - first;
		const glm::dvec3 from_first = point - first;
		const double first_dot = glm::dot(first_edge, from_first);
		const double second_dot = glm::dot(second_edge, from_first);
		if (first_dot <= 0.0 && second_dot <= 0.0) return first;
		const glm::dvec3 from_second = point - second;
		const double third_dot = glm::dot(first_edge, from_second);
		const double fourth_dot = glm::dot(second_edge, from_second);
		if (third_dot >= 0.0 && fourth_dot <= third_dot) return second;
		const double third_region = first_dot * fourth_dot - third_dot * second_dot;
		if (third_region <= 0.0 && first_dot >= 0.0 && third_dot <= 0.0)
			return first + first_dot / (first_dot - third_dot) * first_edge;
		const glm::dvec3 from_third = point - third;
		const double fifth_dot = glm::dot(first_edge, from_third);
		const double sixth_dot = glm::dot(second_edge, from_third);
		if (sixth_dot >= 0.0 && fifth_dot <= sixth_dot) return third;
		const double second_region = fifth_dot * second_dot - first_dot * sixth_dot;
		if (second_region <= 0.0 && second_dot >= 0.0 && sixth_dot <= 0.0)
			return first + second_dot / (second_dot - sixth_dot) * second_edge;
		const double first_region = third_dot * sixth_dot - fifth_dot * fourth_dot;
		if (first_region <= 0.0 && fourth_dot - third_dot >= 0.0 && fifth_dot - sixth_dot >= 0.0)
			return second + (fourth_dot - third_dot) /
				((fourth_dot - third_dot) + (fifth_dot - sixth_dot)) * (third - second);
		const double inverse_sum = 1.0 / (first_region + second_region + third_region);
		return first + second_region * inverse_sum * first_edge +
			third_region * inverse_sum * second_edge;
	}
};

class FacialElasticSurfaceVerifier
{
public:
	double verify(const GeneratedPrimitiveMesh &generated,
		const FacialSkinLandmarks &landmarks, const std::string &label, bool default_detail = true) const
	{
		FacialElasticTestAssertions::require(static_cast<bool>(generated.mesh()), label + ": missing mesh");
		const Mesh &mesh = *generated.mesh();
		const auto *face_mesh = dynamic_cast<const FacialSkinMesh *>(&mesh);
		FacialElasticTestAssertions::require(face_mesh && face_mesh->mechanical_state,
			label + ": missing neutral mechanical reference");
		const auto &rest = face_mesh->mechanical_state->system.rest_positions;
		FacialElasticTestAssertions::require(!mesh.vertices.empty() && !mesh.faces.empty(), label + ": empty skin surface");
		if (default_detail) {
			FacialElasticTestAssertions::require(mesh.vertices.size() == 3034 && mesh.faces.size() == 5712,
				label + ": expected unchanged 48 by 64 native topology");
		}
		FacialElasticTestAssertions::require(mesh.normals.size() == mesh.vertices.size(),
			label + ": missing native normals");
		FacialElasticTestAssertions::require(mesh.texcoords.size() == mesh.vertices.size(),
			label + ": missing material coordinates");
		FacialElasticTestAssertions::require(generated.faceSurfaceTags().size() == mesh.faces.size(),
			label + ": missing face surface tags");
		for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
			const glm::vec3 &position = mesh.vertices[index];
			const glm::vec3 &normal = mesh.normals[index];
			FacialElasticTestAssertions::require(
				std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z),
				label + ": nonfinite vertex");
			FacialElasticTestAssertions::require(
				std::isfinite(normal.x) && std::isfinite(normal.y) && std::isfinite(normal.z) &&
				std::fabs(glm::length(normal) - 1.0f) < 2.0e-5f,
				label + ": nonfinite or nonunit normal");
		}
		for (std::size_t index = 0; index < mesh.faces.size(); ++index) {
			const glm::ivec3 &face = mesh.faces[index];
			for (int vertex : {face.x, face.y, face.z}) {
				FacialElasticTestAssertions::require(vertex >= 0 &&
					static_cast<std::size_t>(vertex) < mesh.vertices.size(), label + ": invalid face index");
			}
			const glm::dvec3 cross = glm::cross(
				glm::dvec3(mesh.vertices[face.y]) - glm::dvec3(mesh.vertices[face.x]),
				glm::dvec3(mesh.vertices[face.z]) - glm::dvec3(mesh.vertices[face.x]));
			FacialElasticTestAssertions::require(glm::length(cross) > 2.56e-13,
				label + ": collapsed triangle " + std::to_string(index));
			const glm::dvec3 mean_normal = glm::dvec3(mesh.normals[face.x]) +
				glm::dvec3(mesh.normals[face.y]) + glm::dvec3(mesh.normals[face.z]);
			FacialElasticTestAssertions::require(glm::dot(cross, mean_normal) > 0.0,
				label + ": interpolated normal opposes triangle " + std::to_string(index));
			const glm::dvec3 rest_normal = glm::normalize(glm::cross(rest[face.y] - rest[face.x],
				rest[face.z] - rest[face.x]));
			FacialElasticTestAssertions::require(glm::dot(cross, rest_normal) > 2.56e-12,
				label + ": triangle exceeded its neutral-normal hemisphere " + std::to_string(index));
		}
		const double minimum_clearance = FacialTriangleGlobeClearanceEvaluator().evaluate(mesh, landmarks);
		FacialElasticTestAssertions::require(minimum_clearance >= -landmarks.eyeball_radius * 5.0e-6,
			label + ": a complete skin facet intersects an eyeball");
		return minimum_clearance;
	}

	void verifySameTopology(const GeneratedPrimitiveMesh &reference,
		const GeneratedPrimitiveMesh &deformed, const std::string &label) const
	{
		const Mesh &first = *reference.mesh();
		const Mesh &second = *deformed.mesh();
		FacialElasticTestAssertions::require(first.faces == second.faces &&
			first.texcoords == second.texcoords && first.vertices.size() == second.vertices.size(),
			label + ": deformation changed material topology");
		for (std::size_t index = 0; index < reference.faceSurfaceTags().size(); ++index) {
			FacialElasticTestAssertions::require(reference.faceSurfaceTags()[index].boundaryIndex() ==
				deformed.faceSurfaceTags()[index].boundaryIndex(), label + ": aperture tags changed");
		}
	}

	void verifyOpenConnectedPatch(const Mesh &mesh) const
	{
		std::map<std::pair<int, int>, int> edge_counts;
		std::vector<std::vector<int>> adjacency(mesh.vertices.size());
		for (const glm::ivec3 &face : mesh.faces) {
			for (const auto edge : {std::make_pair(face.x, face.y),
				std::make_pair(face.y, face.z), std::make_pair(face.z, face.x)}) {
				++edge_counts[std::minmax(edge.first, edge.second)];
				adjacency[edge.first].push_back(edge.second);
				adjacency[edge.second].push_back(edge.first);
			}
		}
		FacialElasticTestAssertions::require(countComponents(adjacency, false) == 1,
			"elastic patch must remain one connected surface");
		std::vector<std::vector<int>> boundary_adjacency(mesh.vertices.size());
		for (const auto &entry : edge_counts) {
			FacialElasticTestAssertions::require(entry.second <= 2, "nonmanifold elastic edge");
			if (entry.second != 1) continue;
			boundary_adjacency[entry.first.first].push_back(entry.first.second);
			boundary_adjacency[entry.first.second].push_back(entry.first.first);
		}
		for (const auto &neighbors : boundary_adjacency) {
			FacialElasticTestAssertions::require(neighbors.empty() || neighbors.size() == 2,
				"elastic patch has a broken boundary loop");
		}
		FacialElasticTestAssertions::require(countComponents(boundary_adjacency, true) == 4,
			"elastic patch requires outline, two eyes and mouth boundaries; it is not a closed volume");
	}

private:
	static int countComponents(const std::vector<std::vector<int>> &adjacency, bool skip_isolated)
	{
		std::vector<bool> visited(adjacency.size(), false);
		int count = 0;
		for (std::size_t first = 0; first < adjacency.size(); ++first) {
			if (visited[first] || (skip_isolated && adjacency[first].empty())) continue;
			++count;
			std::queue<int> pending;
			pending.push(static_cast<int>(first));
			visited[first] = true;
			while (!pending.empty()) {
				const int current = pending.front();
				pending.pop();
				for (int neighbor : adjacency[current]) {
					if (visited[neighbor]) continue;
					visited[neighbor] = true;
					pending.push(neighbor);
				}
			}
		}
		return count;
	}
};

class FacialElasticLandmarkVerifier
{
public:
	void verify(const GeneratedPrimitiveMesh &generated,
		const FacialSkinLandmarks &landmarks, const std::string &label) const
	{
		struct LandmarkVertexCorrespondence {
			const char *name;
			glm::vec2 material_coordinate;
			glm::vec3 position;
		};
		const std::array<LandmarkVertexCorrespondence, 12> correspondences = {{
			{"left outer eye", {40.0f / 48.0f, 42.0f / 64.0f}, landmarks.left_eye_outer_corner},
			{"left inner eye", {28.0f / 48.0f, 42.0f / 64.0f}, landmarks.left_eye_inner_corner},
			{"left upper eye", {34.0f / 48.0f, 46.0f / 64.0f}, landmarks.left_eye_upper_margin},
			{"left lower eye", {34.0f / 48.0f, 38.0f / 64.0f}, landmarks.left_eye_lower_margin},
			{"right outer eye", {8.0f / 48.0f, 42.0f / 64.0f}, landmarks.right_eye_outer_corner},
			{"right inner eye", {20.0f / 48.0f, 42.0f / 64.0f}, landmarks.right_eye_inner_corner},
			{"right upper eye", {14.0f / 48.0f, 46.0f / 64.0f}, landmarks.right_eye_upper_margin},
			{"right lower eye", {14.0f / 48.0f, 38.0f / 64.0f}, landmarks.right_eye_lower_margin},
			{"left mouth", {34.0f / 48.0f, 20.0f / 64.0f}, landmarks.mouth_left_corner},
			{"right mouth", {14.0f / 48.0f, 20.0f / 64.0f}, landmarks.mouth_right_corner},
			{"upper mouth", {24.0f / 48.0f, 24.0f / 64.0f}, landmarks.mouth_upper_margin},
			{"lower mouth", {24.0f / 48.0f, 16.0f / 64.0f}, landmarks.mouth_lower_margin}
		}};
		const Mesh &mesh = *generated.mesh();
		std::vector<bool> front_vertices(mesh.vertices.size(), false);
		for (std::size_t index = 0; index < mesh.faces.size(); ++index) {
			if (generated.faceSurfaceTags()[index].boundaryIndex() != 0) continue;
			for (int vertex : {mesh.faces[index].x, mesh.faces[index].y, mesh.faces[index].z})
				front_vertices[vertex] = true;
		}
		for (const auto &landmark : correspondences) {
			std::size_t match = mesh.vertices.size();
			for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
				if (front_vertices[index] &&
					glm::length(glm::vec2(mesh.texcoords[index]) - landmark.material_coordinate) < 1.0e-7f) {
					FacialElasticTestAssertions::require(match == mesh.vertices.size(),
						label + ": ambiguous material landmark " + landmark.name);
					match = index;
				}
			}
			FacialElasticTestAssertions::require(match < mesh.vertices.size(),
				label + ": missing material landmark " + landmark.name);
			FacialElasticTestAssertions::require(glm::length(mesh.vertices[match] - landmark.position) < 1.0e-7f,
				label + ": landmark is not on its actual solved vertex: " + landmark.name);
		}
	}
};

class FacialElasticTestSpecificationFactory
{
public:
	FacialSkinShapeSpecificationCandidate create(float left_activation, float right_activation) const
	{
		FacialSkinShapeSpecificationCandidate candidate;
		candidate.response_mode = FacialSkinResponseMode::Elastic;
		// This explicit synthetic foundation and modest shortening define the
		// demonstrated regime; they are not anatomical material calibration.
		candidate.elastic_material.support_stiffness_n_per_m3 = 1000000.0f;
		// The integration assessment has an explicit budget. It does not weaken
		// the force tolerance or redefine convergence when the budget runs out.
		candidate.elastic_solve_settings.maximum_iterations = 2000;
		candidate.elastic_solve_settings.relative_force_tolerance = 1.0e-4f;
		candidate.muscles = {
			{"left_smile", {0.87f, 0.56f}, {34.0f / 48.0f, 20.0f / 64.0f},
				{0.055f, 0.08f}, left_activation, 0.04f, 40.0f},
			{"right_smile", {0.13f, 0.56f}, {14.0f / 48.0f, 20.0f / 64.0f},
				{0.055f, 0.08f}, right_activation, 0.04f, 40.0f}};
		return candidate;
	}

	std::shared_ptr<const FacialSkinShapeSpecification> validate(
		const FacialSkinShapeSpecificationCandidate &candidate) const
	{
		std::string diagnostic;
		auto specification = FacialSkinSpecificationValidator().validate(candidate, &diagnostic);
		FacialElasticTestAssertions::require(static_cast<bool>(specification), diagnostic);
		return specification;
	}

	void verifyRejectedConfigurations() const
	{
		FacialSkinSpecificationValidator validator;
		std::string diagnostic;
		for (int control_index = 0; control_index < 7; ++control_index) {
			auto candidate = create(0.0f, 0.0f);
			std::array<float *, 7> controls = {{&candidate.controls.smile_left,
				&candidate.controls.smile_right, &candidate.controls.jaw_open,
				&candidate.controls.blink_left, &candidate.controls.blink_right,
				&candidate.controls.brow_raise_left, &candidate.controls.brow_raise_right}};
			*controls[control_index] = 0.1f;
			FacialElasticTestAssertions::require(!validator.validate(candidate, &diagnostic) && !diagnostic.empty(),
				"elastic mode silently accepted an unsupported analytic control");
		}
		auto candidate = create(0.0f, 0.0f);
		candidate.response_mode = FacialSkinResponseMode::Analytic;
		FacialElasticTestAssertions::require(!validator.validate(candidate, &diagnostic),
			"analytic mode silently ignored mechanical declarations");
		candidate = create(0.0f, 0.0f);
		candidate.elastic_material.young_modulus_pa = 0.0f;
		FacialElasticTestAssertions::require(!validator.validate(candidate, &diagnostic),
			"nonpositive elastic modulus was accepted");
		candidate = create(0.0f, 0.0f);
		candidate.muscles[0].activation = std::numeric_limits<float>::quiet_NaN();
		FacialElasticTestAssertions::require(!validator.validate(candidate, &diagnostic),
			"nonfinite muscle activation was accepted");
	}
};

class FacialElasticPoseSample
{
public:
	FacialElasticPoseSample(std::string sample_label,
		std::shared_ptr<const FacialSkinShapeSpecification> sample_specification,
		GeneratedPrimitiveMesh sample_geometry)
		: label(std::move(sample_label)), specification(std::move(sample_specification)),
		  geometry(std::move(sample_geometry))
	{
		face = dynamic_cast<const FacialSkinMesh *>(geometry.mesh().get());
		FacialElasticTestAssertions::require(face != nullptr, label + ": missing native face subtype");
		FacialElasticTestAssertions::require(static_cast<bool>(face->mechanical_state),
			label + ": missing assembled mechanical state");
	}

	std::string label;
	std::shared_ptr<const FacialSkinShapeSpecification> specification;
	GeneratedPrimitiveMesh geometry;
	const FacialSkinMesh *face = nullptr;
};

class FacialElasticMechanicalVerifier
{
public:
	void verify(const FacialElasticPoseSample &sample) const
	{
		const auto &state = *sample.face->mechanical_state;
		const auto &system = state.system;
		const auto &result = state.solve_result;
		const std::string prefix = sample.label + ": ";
		const std::size_t vertex_count = sample.face->vertices.size();
		FacialElasticTestAssertions::require(result.converged && result.admissible,
			prefix + "mechanical solve did not converge: " + result.termination_reason);
		FacialElasticTestAssertions::require(result.positions.size() == vertex_count &&
			system.rest_positions.size() == vertex_count && system.fixed_vertices.size() == vertex_count,
			prefix + "mechanical arrays do not match native vertices");
		FacialElasticTestAssertions::require(result.relative_free_force <=
			sample.specification->elasticSolveSettings().relative_force_tolerance * (1.0 + 1.0e-9),
			prefix + "reported force residual exceeds the requested tolerance");
		FacialElasticTestAssertions::require(!result.accepted_energies_j.empty(),
			prefix + "missing accepted static energy history");
		for (std::size_t index = 1; index < result.accepted_energies_j.size(); ++index) {
			FacialElasticTestAssertions::require(result.accepted_energies_j[index] <=
				result.accepted_energies_j[index - 1] + 1.0e-14,
				prefix + "accepted static solve step increased energy");
		}
		const auto evaluated = FacialElasticSolver().evaluate(system, result.positions);
		FacialElasticTestAssertions::require(evaluated.gradient_n.size() == vertex_count &&
			evaluated.support_forces_n.size() == vertex_count, prefix + "missing assembled force data");
		FacialElasticTestAssertions::require(std::isfinite(evaluated.total_energy_j) &&
			std::fabs(evaluated.total_energy_j - result.final_evaluation.total_energy_j) <
			1.0e-13 + 1.0e-10 * std::fabs(evaluated.total_energy_j),
			prefix + "reported energy does not describe the actual solved coordinates");
		FacialElasticTestAssertions::require(std::fabs(evaluated.total_energy_j -
			result.accepted_energies_j.back()) < 1.0e-13 + 1.0e-10 * std::fabs(evaluated.total_energy_j),
			prefix + "accepted energy history does not end at the actual solution");
		FacialElasticTestAssertions::require(evaluated.membrane_energy_j >= -1.0e-15 &&
			evaluated.support_energy_j >= 0.0 && evaluated.muscle_energy_j >= 0.0,
			prefix + "a positive elastic energy term became negative");
		FacialElasticTestAssertions::require(std::isfinite(evaluated.maximum_green_strain) &&
			evaluated.maximum_green_strain <= 0.35, prefix + "solution exceeds the declared StVK strain envelope");
		double free_force_squared = 0.0;
		std::size_t free_count = 0;
		glm::dvec3 total_fixed_reaction(0.0);
		glm::dvec3 total_applied_force(0.0);
		for (std::size_t index = 0; index < vertex_count; ++index) {
			FacialElasticTestAssertions::require(sample.face->vertices[index] == glm::vec3(result.positions[index]),
				prefix + "native surface was changed after the elastic solve");
			const glm::vec2 uv(sample.face->texcoords[index]);
			if (uv.x == 0.0f || uv.x == 1.0f || uv.y == 0.0f || uv.y == 1.0f) {
				FacialElasticTestAssertions::require(system.fixed_vertices[index],
					prefix + "neutral outer perimeter is not attached");
			}
			if (system.fixed_vertices[index]) {
				FacialElasticTestAssertions::require(result.positions[index] == system.rest_positions[index],
					prefix + "a fixed protective vertex moved");
				total_fixed_reaction += evaluated.gradient_n[index];
			} else {
				free_force_squared += glm::dot(evaluated.gradient_n[index], evaluated.gradient_n[index]);
				++free_count;
			}
			total_applied_force += evaluated.support_forces_n[index];
			if (!system.external_forces_n.empty()) total_applied_force += system.external_forces_n[index];
		}
		FacialElasticTestAssertions::require(free_count > 0 && free_count < vertex_count,
			prefix + "model requires both deformable and attached vertices");
		for (std::size_t index = 0; index < sample.face->faces.size(); ++index) {
			const unsigned boundary = sample.geometry.faceSurfaceTags()[index].boundaryIndex();
			if (boundary != 1 && boundary != 2) continue;
			for (int vertex : {sample.face->faces[index].x, sample.face->faces[index].y, sample.face->faces[index].z})
				FacialElasticTestAssertions::require(system.fixed_vertices[vertex],
					prefix + "neutral eye margin is not protected by a fixed attachment");
		}
		const double free_force = std::sqrt(free_force_squared);
		FacialElasticTestAssertions::require(std::fabs(free_force - result.free_force_l2_n) <
			1.0e-12 + 1.0e-9 * free_force, prefix + "reported residual does not equal the free energy gradient");
		if (result.force_scale_n > 0.0) {
			FacialElasticTestAssertions::require(std::fabs(free_force / result.force_scale_n -
				result.relative_free_force) < 1.0e-10, prefix + "relative residual has inconsistent force units");
		}
		FacialElasticTestAssertions::require(system.muscles.size() == evaluated.muscle_states.size() &&
			system.muscles.size() == sample.specification->muscles().size(),
			prefix + "muscle diagnostics do not match declarations");
		for (std::size_t muscle_index = 0; muscle_index < system.muscles.size(); ++muscle_index) {
			const auto &muscle = system.muscles[muscle_index];
			const auto &muscle_state = evaluated.muscle_states[muscle_index];
			const auto &declaration = sample.specification->muscles()[muscle_index];
			FacialElasticTestAssertions::require(muscle.name == declaration.name &&
				muscle.activation == declaration.activation &&
				muscle.maximum_shortening == declaration.maximum_shortening &&
				muscle.stiffness_n_per_m == declaration.stiffness_n_per_m,
				prefix + "native assembly changed an explicit muscle declaration");
			double total_weight = 0.0;
			glm::dvec3 neutral_insertion(0.0);
			glm::dvec3 solved_insertion(0.0);
			for (const auto &insertion : muscle.insertions) {
				FacialElasticTestAssertions::require(insertion.vertex_index < vertex_count && insertion.weight >= 0.0,
					prefix + "invalid muscle insertion weight");
				total_weight += insertion.weight;
				neutral_insertion += insertion.weight * system.rest_positions[insertion.vertex_index];
				solved_insertion += insertion.weight * result.positions[insertion.vertex_index];
			}
			FacialElasticTestAssertions::require(std::fabs(total_weight - 1.0) < 1.0e-10,
				prefix + "muscle insertion patch is not normalized");
			FacialElasticTestAssertions::require(std::fabs(glm::length(neutral_insertion - muscle.origin) -
				muscle.rest_length_m) < 1.0e-12, prefix + "muscle length was not measured from the neutral material state");
			FacialElasticTestAssertions::require(glm::length(solved_insertion - muscle_state.insertion_position) < 1.0e-12,
				prefix + "muscle diagnostic insertion does not follow its solved material patch");
			const double length = glm::length(solved_insertion - muscle.origin);
			const double target = muscle.rest_length_m * (1.0 - muscle.activation * muscle.maximum_shortening);
			const double expected_tension = muscle.stiffness_n_per_m * std::max(0.0, length - target);
			FacialElasticTestAssertions::require(std::fabs(muscle_state.target_length_m - target) < 1.0e-12 &&
				std::fabs(muscle_state.tension_n - expected_tension) < 1.0e-10,
				prefix + "muscle response is not the declared tensile cable law");
			if (length > 0.0) total_applied_force -= expected_tension * (solved_insertion - muscle.origin) / length;
		}
		FacialElasticTestAssertions::require(glm::length(total_applied_force + total_fixed_reaction) <=
			free_force * std::sqrt(static_cast<double>(free_count)) * 1.001 + 1.0e-9,
			prefix + "support, muscle and fixed-vertex forces do not balance to the measured residual");
	}
};

class FacialElasticPoseEvaluator
{
public:
	FacialElasticPoseSample evaluate(const std::string &label,
		const FacialSkinShapeSpecificationCandidate &candidate) const
	{
		auto specification = FacialElasticTestSpecificationFactory().validate(candidate);
		std::string diagnostic;
		std::cout << "Solving " << label << "..." << std::endl;
		auto generated = FacialSkinMeshGenerator().generate(*specification, &diagnostic);
		FacialElasticTestAssertions::require(generated.mesh() && !generated.mesh()->vertices.empty(),
			label + ": " + diagnostic);
		FacialElasticPoseSample sample(label, std::move(specification), std::move(generated));
		const double clearance = FacialElasticSurfaceVerifier().verify(sample.geometry, sample.face->landmarks, label,
			candidate.horizontal_segments == 48 && candidate.vertical_segments == 64);
		FacialElasticLandmarkVerifier().verify(sample.geometry, sample.face->landmarks, label);
		FacialElasticMechanicalVerifier().verify(sample);
		const auto &result = sample.face->mechanical_state->solve_result;
		const auto &system = sample.face->mechanical_state->system;
		const std::vector<glm::dvec3> exported_positions(sample.face->vertices.begin(), sample.face->vertices.end());
		const auto exported_evaluation = FacialElasticSolver().evaluate(system, exported_positions);
		double exported_force_squared = 0.0;
		for (std::size_t index = 0; index < exported_positions.size(); ++index) {
			if (!system.fixed_vertices[index])
				exported_force_squared += glm::dot(exported_evaluation.gradient_n[index], exported_evaluation.gradient_n[index]);
		}
		const double exported_force = std::sqrt(exported_force_squared);
		std::cout << "  iterations=" << result.iterations << " relative_force=" << result.relative_free_force
			<< " energy_J=" << result.final_evaluation.total_energy_j
			<< " minimum_globe_clearance_m=" << clearance
			<< " float_mesh_free_force_N=" << exported_force
			<< " float_mesh_relative_force=" << (result.force_scale_n > 0.0 ? exported_force / result.force_scale_n : 0.0)
			<< '\n';
		return sample;
	}
};

class FacialElasticResponseComparison
{
public:
	double maximumDisplacement(const Mesh &reference, const Mesh &deformed) const
	{
		FacialElasticTestAssertions::require(reference.vertices.size() == deformed.vertices.size(),
			"cannot compare deformations with different topology");
		double maximum = 0.0;
		for (std::size_t index = 0; index < reference.vertices.size(); ++index) {
			maximum = std::max(maximum, glm::length(glm::dvec3(deformed.vertices[index]) -
				glm::dvec3(reference.vertices[index])));
		}
		return maximum;
	}

	double muscleShortening(const FacialElasticPoseSample &sample, std::size_t muscle_index) const
	{
		const auto &state = *sample.face->mechanical_state;
		return state.system.muscles.at(muscle_index).rest_length_m -
			state.solve_result.final_evaluation.muscle_states.at(muscle_index).length_m;
	}

	double totalMuscleShortening(const FacialElasticPoseSample &sample) const
	{
		double total = 0.0;
		for (std::size_t index = 0; index < sample.specification->muscles().size(); ++index)
			total += muscleShortening(sample, index);
		return total;
	}

	double maximumReferenceDisplacement(const FacialElasticPoseSample &sample) const
	{
		const auto &state = *sample.face->mechanical_state;
		double maximum = 0.0;
		for (std::size_t index = 0; index < state.solve_result.positions.size(); ++index)
			maximum = std::max(maximum, glm::length(state.solve_result.positions[index] - state.system.rest_positions[index]));
		return maximum;
	}

	void verifyDeterministic(const FacialElasticPoseSample &first, const FacialElasticPoseSample &second) const
	{
		const auto &first_result = first.face->mechanical_state->solve_result;
		const auto &second_result = second.face->mechanical_state->solve_result;
		FacialElasticTestAssertions::require(first.face->vertices == second.face->vertices &&
			first.face->normals == second.face->normals && first_result.positions == second_result.positions &&
			first_result.accepted_energies_j == second_result.accepted_energies_j &&
			first_result.relative_free_force == second_result.relative_free_force &&
			first_result.iterations == second_result.iterations,
			second.label + ": solve depends on previous pose queries");
	}

	double mirrorDisplacementError(const FacialElasticPoseSample &neutral,
		const FacialElasticPoseSample &left, const FacialElasticPoseSample &right) const
	{
		const Mesh &rest = *neutral.face;
		std::vector<bool> front(rest.vertices.size(), false);
		for (std::size_t index = 0; index < rest.faces.size(); ++index) {
			if (neutral.geometry.faceSurfaceTags()[index].boundaryIndex() != 0) continue;
			for (int vertex : {rest.faces[index].x, rest.faces[index].y, rest.faces[index].z}) front[vertex] = true;
		}
		std::map<std::array<int, 3>, std::size_t> material_indices;
		for (std::size_t index = 0; index < rest.vertices.size(); ++index) {
			const auto &uv = rest.texcoords[index];
			material_indices[{{static_cast<int>(std::lround(uv.x * 48.0f)),
				static_cast<int>(std::lround(uv.y * 64.0f)), front[index] ? 1 : 0}}] = index;
		}
		double squared_difference = 0.0;
		double squared_magnitude = 0.0;
		for (std::size_t index = 0; index < rest.vertices.size(); ++index) {
			const auto &uv = rest.texcoords[index];
			const std::array<int, 3> mirror_key = {{48 - static_cast<int>(std::lround(uv.x * 48.0f)),
				static_cast<int>(std::lround(uv.y * 64.0f)), front[index] ? 1 : 0}};
			const auto counterpart = material_indices.find(mirror_key);
			FacialElasticTestAssertions::require(counterpart != material_indices.end(),
				"missing mirrored material vertex");
			const std::size_t mirror_index = counterpart->second;
			const glm::dvec3 first = glm::dvec3(left.face->vertices[index]) - glm::dvec3(rest.vertices[index]);
			const glm::dvec3 second = glm::dvec3(right.face->vertices[mirror_index]) - glm::dvec3(rest.vertices[mirror_index]);
			const glm::dvec3 difference = glm::dvec3(-first.x, first.y, first.z) - second;
			squared_difference += glm::dot(difference, difference);
			squared_magnitude += glm::dot(first, first);
		}
		FacialElasticTestAssertions::require(squared_magnitude > 0.0, "unilateral activation did not deform skin");
		return std::sqrt(squared_difference / squared_magnitude);
	}

	void verifyMaterialAndAttachmentAssembly(const FacialElasticPoseSample &reference,
		const FacialElasticPoseSample &stiffer, const FacialElasticPoseSample &mapped,
		const FacialElasticPoseSample &attached, double attachment_stiffness) const
	{
		const auto &base_state = *reference.face->mechanical_state;
		const auto &stiff_state = *stiffer.face->mechanical_state;
		const auto &mapped_state = *mapped.face->mechanical_state;
		const auto &attached_state = *attached.face->mechanical_state;
		FacialElasticTestAssertions::require(base_state.system.triangles.size() == stiff_state.system.triangles.size() &&
			base_state.system.triangles.size() == mapped_state.system.triangles.size(),
			"changing a material changed the membrane topology");
		bool mapped_triangle_changed = false;
		bool untouched_triangle_present = false;
		for (std::size_t index = 0; index < base_state.system.triangles.size(); ++index) {
			const double base_modulus = base_state.system.triangles[index].young_modulus_pa;
			FacialElasticTestAssertions::require(stiff_state.system.triangles[index].young_modulus_pa == 2.0 * base_modulus,
				"doubling skin modulus did not double the actual membrane coefficients");
			const double mapped_modulus = mapped_state.system.triangles[index].young_modulus_pa;
			mapped_triangle_changed |= mapped_modulus > base_modulus * 1.01;
			untouched_triangle_present |= mapped_modulus == base_modulus;
		}
		FacialElasticTestAssertions::require(mapped_triangle_changed && untouched_triangle_present,
			"local tissue region did not change a bounded part of the actual membrane");
		const std::size_t count = reference.face->vertices.size();
		FacialElasticTestAssertions::require(base_state.rest_vertex_areas_m2.size() == count &&
			mapped_state.young_modulus_scales.size() == count && mapped_state.support_stiffness_scales.size() == count,
			"material map diagnostics do not match native vertices");
		double added_stiffness = 0.0;
		bool mapped_support_changed = false;
		for (std::size_t index = 0; index < count; ++index) {
			const double area = base_state.rest_vertex_areas_m2[index];
			const double base_stiffness = base_state.system.support_stiffness_n_per_m[index];
			const double expected = area * reference.specification->elasticMaterial().support_stiffness_n_per_m3;
			FacialElasticTestAssertions::require(area > 0.0 && std::fabs(base_stiffness - expected) < 1.0e-12,
				"foundation stiffness is not integrated over neutral nodal surface area");
			FacialElasticTestAssertions::require(stiff_state.system.support_stiffness_n_per_m[index] == base_stiffness,
				"changing Young's modulus also changed unrelated foundation stiffness");
			const double mapped_scale = mapped_state.support_stiffness_scales[index];
			FacialElasticTestAssertions::require(mapped_scale >= 0.1 && mapped_scale <= 10.0 &&
				mapped_state.young_modulus_scales[index] >= 0.1 && mapped_state.young_modulus_scales[index] <= 10.0,
				"material region escaped its declared scale bounds");
			FacialElasticTestAssertions::require(std::fabs(mapped_state.system.support_stiffness_n_per_m[index] -
				base_stiffness * mapped_scale) < 1.0e-11, "support map is metadata only; it was not assembled");
			mapped_support_changed |= mapped_scale > 1.01;
			const double difference = attached_state.system.support_stiffness_n_per_m[index] - base_stiffness;
			FacialElasticTestAssertions::require(difference >= -1.0e-12, "adding an attachment weakened foundation support");
			added_stiffness += difference;
		}
		FacialElasticTestAssertions::require(mapped_support_changed, "support material map did not affect any node");
		FacialElasticTestAssertions::require(std::fabs(added_stiffness - attachment_stiffness) < 1.0e-8,
			"attachment did not preserve its declared total patch stiffness in N/m");
	}
};

class FacialElasticAdmissibilityVerifier
{
public:
	void verify(const FacialElasticPoseSample &neutral) const
	{
		// This native lower lip triangle is initially steep: its unit normal's
		// Z component is about .159. A mild rigid tilt crosses world Z=0 without
		// either material strain or triangle degeneration.
		const glm::ivec3 native_face = neutral.face->faces.at(1733);
		Mesh triangle;
		for (int index : {native_face.x, native_face.y, native_face.z})
			triangle.vertices.push_back(neutral.face->vertices[index]);
		triangle.faces.emplace_back(0, 1, 2);
		const std::vector<MeshSurfaceTag> tags{MeshSurfaceTag(MeshSurfaceRole::Outer, 0)};
		const std::vector<bool> moving_vertices(3, false);
		const std::vector<glm::dvec3> rest(triangle.vertices.begin(), triangle.vertices.end());
		const glm::dvec3 center = (rest[0] + rest[1] + rest[2]) / 3.0;
		FacialSkinAdmissibility guard(triangle, tags, neutral.face->landmarks, moving_vertices, 0.16);
		FacialElasticTestAssertions::require(guard.accepts(rest), "native neutral lip fixture is inadmissible");
		const auto mild_rotation = rotateAroundX(rest, center, 0.20);
		const glm::dvec3 mild_cross = glm::cross(mild_rotation[1] - mild_rotation[0], mild_rotation[2] - mild_rotation[0]);
		FacialElasticTestAssertions::require(mild_cross.z < 0.0 && guard.accepts(mild_rotation),
			"elastic geometry guard incorrectly rejects a mild steep-lip rotation using world Z");
		FacialElasticSystem triangle_system;
		triangle_system.rest_positions = rest;
		triangle_system.triangles.push_back(FacialElasticSolver::makeRestTriangle(
			{{0, 1, 2}}, rest, 50000.0, 0.35, 0.001));
		const auto reversed_rotation = rotateAroundX(rest, center, 3.14159265358979323846);
		FacialElasticTestAssertions::require(FacialElasticSolver().evaluate(triangle_system, mild_rotation).membrane_energy_j < 1.0e-20 &&
			FacialElasticSolver().evaluate(triangle_system, reversed_rotation).membrane_energy_j < 1.0e-20,
			"rigid rotation introduced membrane strain in the geometric guard fixture");
		// The hemisphere limit is an explicit geometry heuristic. It may reject
		// a large rigid rotation even though the constitutive energy is objective.
		FacialElasticTestAssertions::require(!guard.accepts(reversed_rotation),
			"elastic geometry guard accepted a relative normal reversal");
		auto collapsed = rest;
		collapsed[2] = (rest[0] + rest[1]) / 2.0;
		FacialElasticTestAssertions::require(!guard.accepts(collapsed),
			"elastic geometry guard accepted a collapsed triangle");

		const glm::dvec3 rest_normal = glm::normalize(glm::cross(rest[1] - rest[0], rest[2] - rest[0]));
		const glm::dvec3 first_axis = glm::normalize(rest[1] - rest[0]);
		const glm::dvec3 second_axis = glm::cross(rest_normal, first_axis);
		const glm::dvec3 globe_center(neutral.face->landmarks.left_eyeball_center);
		const double globe_radius = neutral.face->landmarks.eyeball_radius;
		std::vector<glm::dvec3> globe_crossing;
		for (int index = 0; index < 3; ++index) {
			const double angle = index * 2.0 * 3.14159265358979323846 / 3.0;
			globe_crossing.push_back(globe_center + 0.5 * globe_radius * rest_normal +
				1.5 * globe_radius * (std::cos(angle) * first_axis + std::sin(angle) * second_axis));
			FacialElasticTestAssertions::require(glm::length(globe_crossing.back() - globe_center) > globe_radius,
				"facet-contact fixture requires all vertices outside the globe");
		}
		FacialElasticTestAssertions::require(!guard.accepts(globe_crossing) &&
			guard.lastRejection().find("globe") != std::string::npos,
			"geometry guard missed a facet crossing a globe with all vertices outside");

		// Moving the entire reference frame (including its globes) must preserve
		// this local geometric acceptance decision.
		const glm::dvec3 translation(0.2, -0.1, 0.3);
		const auto transform = [&translation](const glm::dvec3 &point) {
			const double cosine = std::cos(0.7), sine = std::sin(0.7);
			return glm::dvec3(cosine * point.x + sine * point.z, point.y,
				-sine * point.x + cosine * point.z) + translation;
		};
		Mesh moved_reference;
		moved_reference.faces = triangle.faces;
		std::vector<glm::dvec3> moved_mild, moved_crossing;
		for (std::size_t index = 0; index < rest.size(); ++index) {
			moved_reference.vertices.push_back(glm::vec3(transform(rest[index])));
			moved_mild.push_back(transform(mild_rotation[index]));
			moved_crossing.push_back(transform(globe_crossing[index]));
		}
		auto moved_landmarks = neutral.face->landmarks;
		moved_landmarks.left_eyeball_center = glm::vec3(transform(glm::dvec3(moved_landmarks.left_eyeball_center)));
		moved_landmarks.right_eyeball_center = glm::vec3(transform(glm::dvec3(moved_landmarks.right_eyeball_center)));
		FacialSkinAdmissibility moved_guard(moved_reference, tags, moved_landmarks, moving_vertices, 0.16);
		FacialElasticTestAssertions::require(moved_guard.accepts(moved_mild) && !moved_guard.accepts(moved_crossing),
			"local orientation/contact guard depends on an arbitrary world coordinate frame");
	}

private:
	static std::vector<glm::dvec3> rotateAroundX(const std::vector<glm::dvec3> &positions,
		const glm::dvec3 &center, double radians)
	{
		const double cosine = std::cos(radians), sine = std::sin(radians);
		std::vector<glm::dvec3> rotated;
		for (const auto &position : positions) {
			const glm::dvec3 offset = position - center;
			rotated.push_back(center + glm::dvec3(offset.x, cosine * offset.y - sine * offset.z,
				sine * offset.y + cosine * offset.z));
		}
		return rotated;
	}
};

class FacialElasticIntegrationVerification
{
public:
	void run() const
	{
		FacialElasticTestSpecificationFactory specifications;
		FacialElasticPoseEvaluator evaluator;
		FacialElasticSurfaceVerifier surfaces;
		FacialElasticResponseComparison comparison;
		specifications.verifyRejectedConfigurations();

		auto empty_candidate = specifications.create(0.0f, 0.0f);
		empty_candidate.muscles.clear();
		const auto unloaded = evaluator.evaluate("neutral without muscles", empty_candidate);
		const auto inactive = evaluator.evaluate("neutral with inactive muscles", specifications.create(0.0f, 0.0f));
		surfaces.verifyOpenConnectedPatch(*inactive.face);
		FacialElasticAdmissibilityVerifier().verify(inactive);
		const auto analytic_specification = specifications.validate(FacialSkinShapeSpecificationCandidate{});
		const auto analytic_neutral = FacialSkinMeshGenerator().generate(*analytic_specification, nullptr);
		FacialElasticTestAssertions::require(unloaded.face->vertices == analytic_neutral.mesh()->vertices &&
			inactive.face->vertices == analytic_neutral.mesh()->vertices,
			"unloaded elasticity changed its genuine neutral material reference");
		FacialElasticTestAssertions::require(inactive.face->mechanical_state->solve_result.final_evaluation.total_energy_j < 1.0e-18,
			"neutral reference retains elastic prestress");

		const auto left = evaluator.evaluate("left activation 0.5", specifications.create(0.5f, 0.0f));
		const auto right = evaluator.evaluate("right activation 0.5", specifications.create(0.0f, 0.5f));
		const auto quarter = evaluator.evaluate("bilateral activation 0.25", specifications.create(0.25f, 0.25f));
		const auto half = evaluator.evaluate("bilateral activation 0.5", specifications.create(0.5f, 0.5f));
		FacialElasticTestAssertions::require(comparison.muscleShortening(left, 0) > 1.0e-7 &&
			comparison.muscleShortening(right, 1) > 1.0e-7,
			"unilateral cable activation did not pull its insertion towards its skull origin");
		const glm::vec3 left_motion = left.face->landmarks.mouth_left_corner - inactive.face->landmarks.mouth_left_corner;
		const glm::vec3 right_motion = right.face->landmarks.mouth_right_corner - inactive.face->landmarks.mouth_right_corner;
		FacialElasticTestAssertions::require(left_motion.x > 0.0f && left_motion.y > 0.0f &&
			right_motion.x < 0.0f && right_motion.y > 0.0f,
			"unilateral smile cable did not lift the intended mouth corner outwards");
		const double mirror_error = comparison.mirrorDisplacementError(inactive, left, right);
		// Mirroring changes the grid's diagonal orientation. A mechanics test
		// therefore allows discretization error instead of claiming exact symmetry.
		FacialElasticTestAssertions::require(mirror_error < 0.15,
			"left/right response differs beyond the allowed triangulation error");
		FacialElasticTestAssertions::require(comparison.totalMuscleShortening(half) >
			comparison.totalMuscleShortening(quarter) * 1.01,
			"increasing bounded cable activation did not increase its solved shortening");
		FacialElasticTestAssertions::require(comparison.maximumDisplacement(*inactive.face, *half.face) > 1.0e-6,
			"elastic activation produced no measurable native deformation");

		auto stiff_candidate = specifications.create(0.5f, 0.5f);
		stiff_candidate.elastic_material.young_modulus_pa *= 2.0f;
		const auto stiffer = evaluator.evaluate("twice Young modulus", stiff_candidate);
		auto mapped_candidate = specifications.create(0.5f, 0.5f);
		mapped_candidate.tissue_regions.push_back({"left_cheek_material", {0.71f, 0.36f}, {0.23f, 0.20f}, 4.0f, 3.0f});
		const auto mapped = evaluator.evaluate("local modulus and support map", mapped_candidate);
		auto attached_candidate = specifications.create(0.5f, 0.5f);
		const double attachment_stiffness = 500.0;
		attached_candidate.attachments.push_back({"left_mouth_support", {34.0f / 48.0f, 20.0f / 64.0f},
			{0.09f, 0.10f}, static_cast<float>(attachment_stiffness)});
		const auto attached = evaluator.evaluate("left attachment sensitivity", attached_candidate);
		comparison.verifyMaterialAndAttachmentAssembly(half, stiffer, mapped, attached, attachment_stiffness);
		FacialElasticTestAssertions::require(comparison.totalMuscleShortening(stiffer) <
			comparison.totalMuscleShortening(half), "stiffer membrane did not resist the prescribed cable shortening");
		FacialElasticTestAssertions::require(comparison.maximumDisplacement(*half.face, *mapped.face) > 1.0e-7,
			"changing a material map did not change the mechanical solution");
		const double attached_motion = glm::length(attached.face->landmarks.mouth_left_corner -
			inactive.face->landmarks.mouth_left_corner);
		const double original_motion = glm::length(half.face->landmarks.mouth_left_corner -
			inactive.face->landmarks.mouth_left_corner);
		FacialElasticTestAssertions::require(attached_motion < original_motion,
			"stronger local attachment did not restrain its mouth corner");
		for (const FacialElasticPoseSample *sample : {&left, &right, &quarter, &half, &stiffer, &mapped, &attached}) {
			surfaces.verifySameTopology(inactive.geometry, sample->geometry, sample->label);
			FacialElasticTestAssertions::require(sample->face->mechanical_state->system.rest_positions ==
				inactive.face->mechanical_state->system.rest_positions,
				sample->label + ": the material reference was retargeted to a pose");
		}
		auto refined_candidate = specifications.create(0.5f, 0.0f);
		refined_candidate.horizontal_segments = 96;
		refined_candidate.vertical_segments = 128;
		const auto refined = evaluator.evaluate("96 by 128 left activation 0.5", refined_candidate);
		surfaces.verifyOpenConnectedPatch(*refined.face);
		const double coarse_shortening = comparison.muscleShortening(left, 0);
		const double refined_shortening = comparison.muscleShortening(refined, 0);
		std::cout << "  refinement observation: vertices=" << refined.face->vertices.size()
			<< " triangles=" << refined.face->faces.size()
			<< " coarse_insertion_shortening_m=" << coarse_shortening
			<< " refined_insertion_shortening_m=" << refined_shortening
			<< " coarse_max_displacement_m=" << comparison.maximumReferenceDisplacement(left)
			<< " refined_max_displacement_m=" << comparison.maximumReferenceDisplacement(refined)
			<< " mouth_landmark_difference_m=" << glm::length(refined.face->landmarks.mouth_left_corner -
				left.face->landmarks.mouth_left_corner) << '\n';

		// A reverse traversal and a repeated unilateral query must reproduce the
		// same native solution, with no hidden animation history or warm start.
		const auto quarter_reversed = evaluator.evaluate("reverse query activation 0.25", specifications.create(0.25f, 0.25f));
		const auto left_repeated = evaluator.evaluate("repeat left activation 0.5", specifications.create(0.5f, 0.0f));
		const auto neutral_return = evaluator.evaluate("return to neutral", specifications.create(0.0f, 0.0f));
		comparison.verifyDeterministic(quarter, quarter_reversed);
		comparison.verifyDeterministic(left, left_repeated);
		comparison.verifyDeterministic(inactive, neutral_return);
		FacialElasticLandmarkVerifier().verify(half.geometry,
			FacialSkinMeshGenerator().evaluateLandmarks(*half.specification), "public solved landmark evaluation");
		std::cout << "Facial elastic integration PASS: 3034 vertices, 5712 triangles, four boundaries; "
			<< "maximum bilateral displacement_m=" << comparison.maximumDisplacement(*inactive.face, *half.face)
			<< ", reflected unilateral relative_error=" << mirror_error
			<< ", attachment displacement_ratio=" << attached_motion / original_motion
			<< ", exact repeated/reverse-query geometry and neutral return.\n";
	}
};

} // namespace

int main()
{
	try {
		FacialElasticIntegrationVerification().run();
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "Facial elastic integration FAIL: " << error.what() << '\n';
		return 1;
	}
}
