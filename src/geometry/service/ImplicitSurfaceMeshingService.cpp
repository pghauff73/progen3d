#include "geometry/service/ImplicitSurfaceMeshingService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace {

struct SampledFieldPoint
{
	glm::dvec3 position{0.0};
	double value = 0.0;
};

struct QuantizedPosition
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const QuantizedPosition &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
};

constexpr double kVertexQuantization = 100000000.0;

QuantizedPosition quantizePosition(const glm::dvec3 &position)
{
	return {
		static_cast<std::int64_t>(std::llround(position.x * kVertexQuantization)),
		static_cast<std::int64_t>(std::llround(position.y * kVertexQuantization)),
		static_cast<std::int64_t>(std::llround(position.z * kVertexQuantization))};
}

glm::dvec3 interpolateIsoEdge(
	const SampledFieldPoint &first,
	const SampledFieldPoint &second,
	double iso_value)
{
	const double denominator = second.value - first.value;
	double parameter = 0.5;
	if (std::fabs(denominator) > 1.0e-14) {
		parameter = (iso_value - first.value) / denominator;
	}
	parameter = std::clamp(parameter, 0.0, 1.0);
	return first.position + (second.position - first.position) * parameter;
}

class IsoSurfaceMeshAssembler
{
public:
	int vertexIndex(const glm::dvec3 &position)
	{
		const QuantizedPosition key = quantizePosition(position);
		const auto existing = vertex_indices_.find(key);
		if (existing != vertex_indices_.end()) return existing->second;
		const int index = static_cast<int>(mesh_.vertices.size());
		mesh_.vertices.emplace_back(position);
		vertex_indices_.emplace(key, index);
		return index;
	}

	void addTriangle(
		const glm::dvec3 &first,
		const glm::dvec3 &second,
		const glm::dvec3 &third)
	{
		const int first_index = vertexIndex(first);
		const int second_index = vertexIndex(second);
		const int third_index = vertexIndex(third);
		if (first_index == second_index || second_index == third_index ||
		    third_index == first_index) {
			return;
		}
		const glm::dvec3 normal = glm::cross(second - first, third - first);
		if (!std::isfinite(normal.x) || !std::isfinite(normal.y) ||
		    !std::isfinite(normal.z)) {
			return;
		}
		mesh_.faces.emplace_back(first_index, second_index, third_index);
	}

	Mesh takeMesh() { return std::move(mesh_); }

private:
	Mesh mesh_;
	std::map<QuantizedPosition, int> vertex_indices_;
};

void polygonizeTetrahedron(
	const std::array<SampledFieldPoint, 4> &tetrahedron,
	double iso_value,
	IsoSurfaceMeshAssembler *assembler)
{
	std::vector<int> inside;
	std::vector<int> outside;
	for (int index = 0; index < 4; ++index) {
		(tetrahedron[static_cast<std::size_t>(index)].value <= iso_value
			 ? inside
			 : outside)
			.push_back(index);
	}
	if (inside.empty() || outside.empty()) return;

	if (inside.size() == 1u || outside.size() == 1u) {
		const bool isolated_inside = inside.size() == 1u;
		const int isolated = isolated_inside ? inside.front() : outside.front();
		const std::vector<int> &opposite = isolated_inside ? outside : inside;
		std::array<glm::dvec3, 3> points;
		for (std::size_t index = 0u; index < opposite.size(); ++index) {
			points[index] = interpolateIsoEdge(
				tetrahedron[static_cast<std::size_t>(isolated)],
				tetrahedron[static_cast<std::size_t>(opposite[index])],
				iso_value);
		}
		if (isolated_inside) {
			assembler->addTriangle(points[0], points[1], points[2]);
		}
		else {
			assembler->addTriangle(points[0], points[2], points[1]);
		}
		return;
	}

	const glm::dvec3 first_first = interpolateIsoEdge(
		tetrahedron[static_cast<std::size_t>(inside[0])],
		tetrahedron[static_cast<std::size_t>(outside[0])], iso_value);
	const glm::dvec3 first_second = interpolateIsoEdge(
		tetrahedron[static_cast<std::size_t>(inside[0])],
		tetrahedron[static_cast<std::size_t>(outside[1])], iso_value);
	const glm::dvec3 second_first = interpolateIsoEdge(
		tetrahedron[static_cast<std::size_t>(inside[1])],
		tetrahedron[static_cast<std::size_t>(outside[0])], iso_value);
	const glm::dvec3 second_second = interpolateIsoEdge(
		tetrahedron[static_cast<std::size_t>(inside[1])],
		tetrahedron[static_cast<std::size_t>(outside[1])], iso_value);
	assembler->addTriangle(first_first, second_first, second_second);
	assembler->addTriangle(first_first, second_second, first_second);
}

void smoothMesh(
	Mesh *mesh,
	int iterations,
	double expansion_factor,
	double contraction_factor)
{
	if (mesh == nullptr || mesh->vertices.empty() || iterations <= 0) return;
	std::vector<std::vector<int>> neighbours(mesh->vertices.size());
	for (const glm::ivec3 &face : mesh->faces) {
		const std::array<int, 3> indices{{face.x, face.y, face.z}};
		for (int edge = 0; edge < 3; ++edge) {
			const int first = indices[static_cast<std::size_t>(edge)];
			const int second = indices[static_cast<std::size_t>((edge + 1) % 3)];
			neighbours[static_cast<std::size_t>(first)].push_back(second);
			neighbours[static_cast<std::size_t>(second)].push_back(first);
		}
	}
	for (std::vector<int> &vertex_neighbours : neighbours) {
		std::sort(vertex_neighbours.begin(), vertex_neighbours.end());
		vertex_neighbours.erase(
			std::unique(vertex_neighbours.begin(), vertex_neighbours.end()),
			vertex_neighbours.end());
	}

	auto apply_laplacian_step = [&](double factor) {
		std::vector<glm::vec3> updated = mesh->vertices;
		for (std::size_t index = 0u; index < mesh->vertices.size(); ++index) {
			if (neighbours[index].empty()) continue;
			glm::dvec3 average(0.0);
			for (int neighbour : neighbours[index]) {
				average += glm::dvec3(
					mesh->vertices[static_cast<std::size_t>(neighbour)]);
			}
			average /= static_cast<double>(neighbours[index].size());
			const glm::dvec3 current(mesh->vertices[index]);
			updated[index] = glm::vec3(current + (average - current) * factor);
		}
		mesh->vertices = std::move(updated);
	};

	for (int iteration = 0; iteration < iterations; ++iteration) {
		apply_laplacian_step(expansion_factor);
		apply_laplacian_step(contraction_factor);
	}
}

bool meshIsFinite(const Mesh &mesh)
{
	for (const glm::vec3 &vertex : mesh.vertices) {
		if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
		    !std::isfinite(vertex.z)) {
			return false;
		}
	}
	return !mesh.vertices.empty() && !mesh.faces.empty();
}

ImplicitFieldBounds calculateMeshBounds(const Mesh &mesh)
{
	glm::dvec3 minimum(std::numeric_limits<double>::infinity());
	glm::dvec3 maximum(-std::numeric_limits<double>::infinity());
	for (const glm::vec3 &vertex : mesh.vertices) {
		minimum = glm::min(minimum, glm::dvec3(vertex));
		maximum = glm::max(maximum, glm::dvec3(vertex));
	}
	return ImplicitFieldBounds(minimum, maximum);
}

bool requestIsValid(const ImplicitSurfaceGenerationRequest &request)
{
	return request.firstAxisSamples() >= 3u &&
	       request.secondAxisSamples() >= 3u &&
	       request.thirdAxisSamples() >= 3u &&
	       std::isfinite(request.isoValue()) &&
	       request.smoothingIterations() >= 0 &&
	       std::isfinite(request.smoothingExpansionFactor()) &&
	       std::isfinite(request.smoothingContractionFactor());
}

} // namespace

ImplicitSurfaceGenerationResult ImplicitSurfaceMeshingService::generate(
	const ImplicitScalarField &field,
	const ImplicitSurfaceGenerationRequest &request) const
{
	if (!requestIsValid(request)) {
		return ImplicitSurfaceGenerationResult::createFailure(
			"Implicit surface generation requires finite values and at least three samples per axis.");
	}

	const ImplicitFieldBounds bounds = field.evaluationBounds();
	if (!bounds.isValid()) {
		return ImplicitSurfaceGenerationResult::createFailure(
			"Implicit scalar field evaluation bounds are invalid.");
	}

	const std::size_t first_axis_count = request.firstAxisSamples();
	const std::size_t second_axis_count = request.secondAxisSamples();
	const std::size_t third_axis_count = request.thirdAxisSamples();
	const glm::dvec3 step(
		bounds.dimensions().x / static_cast<double>(first_axis_count - 1u),
		bounds.dimensions().y / static_cast<double>(second_axis_count - 1u),
		bounds.dimensions().z / static_cast<double>(third_axis_count - 1u));

	auto grid_index = [=](
		std::size_t first_axis,
		std::size_t second_axis,
		std::size_t third_axis) {
		return (first_axis * second_axis_count + second_axis) *
			third_axis_count + third_axis;
	};
	std::vector<double> field_values(
		first_axis_count * second_axis_count * third_axis_count, 0.0);
	for (std::size_t first_axis = 0u; first_axis < first_axis_count; ++first_axis) {
		for (std::size_t second_axis = 0u; second_axis < second_axis_count;
		     ++second_axis) {
			for (std::size_t third_axis = 0u; third_axis < third_axis_count;
			     ++third_axis) {
				const glm::dvec3 position = bounds.minimum() + glm::dvec3(
					static_cast<double>(first_axis) * step.x,
					static_cast<double>(second_axis) * step.y,
					static_cast<double>(third_axis) * step.z);
				const double value = field.evaluateAt(position);
				if (!std::isfinite(value)) {
					return ImplicitSurfaceGenerationResult::createFailure(
						"Implicit scalar field produced a non-finite sample.");
				}
				field_values[grid_index(first_axis, second_axis, third_axis)] = value;
			}
		}
	}

	constexpr std::array<std::array<int, 3>, 8> corner_offsets{{
		{{0, 0, 0}}, {{1, 0, 0}}, {{1, 1, 0}}, {{0, 1, 0}},
		{{0, 0, 1}}, {{1, 0, 1}}, {{1, 1, 1}}, {{0, 1, 1}}}};
	constexpr std::array<std::array<int, 4>, 6> tetrahedra{{
		{{0, 1, 2, 6}}, {{0, 2, 3, 6}}, {{0, 3, 7, 6}},
		{{0, 7, 4, 6}}, {{0, 4, 5, 6}}, {{0, 5, 1, 6}}}};
	IsoSurfaceMeshAssembler assembler;
	for (std::size_t first_axis = 0u; first_axis + 1u < first_axis_count;
	     ++first_axis) {
		for (std::size_t second_axis = 0u; second_axis + 1u < second_axis_count;
		     ++second_axis) {
			for (std::size_t third_axis = 0u;
			     third_axis + 1u < third_axis_count; ++third_axis) {
				std::array<SampledFieldPoint, 8> cube;
				for (std::size_t corner = 0u; corner < cube.size(); ++corner) {
					const std::size_t sample_first = first_axis +
						static_cast<std::size_t>(corner_offsets[corner][0]);
					const std::size_t sample_second = second_axis +
						static_cast<std::size_t>(corner_offsets[corner][1]);
					const std::size_t sample_third = third_axis +
						static_cast<std::size_t>(corner_offsets[corner][2]);
					cube[corner] = {
						bounds.minimum() + glm::dvec3(
							static_cast<double>(sample_first) * step.x,
							static_cast<double>(sample_second) * step.y,
							static_cast<double>(sample_third) * step.z),
						field_values[grid_index(
							sample_first, sample_second, sample_third)]};
				}
				for (const std::array<int, 4> &tetrahedron_indices : tetrahedra) {
					std::array<SampledFieldPoint, 4> tetrahedron;
					for (std::size_t index = 0u; index < tetrahedron.size(); ++index) {
						tetrahedron[index] = cube[static_cast<std::size_t>(
							tetrahedron_indices[index])];
					}
					polygonizeTetrahedron(
						tetrahedron, request.isoValue(), &assembler);
				}
			}
		}
	}

	Mesh mesh = assembler.takeMesh();
	if (!meshIsFinite(mesh)) {
		return ImplicitSurfaceGenerationResult::createFailure(
			"Implicit scalar field did not produce a finite surface mesh.");
	}
	smoothMesh(
		&mesh,
		request.smoothingIterations(),
		request.smoothingExpansionFactor(),
		request.smoothingContractionFactor());
	if (!meshIsFinite(mesh)) {
		return ImplicitSurfaceGenerationResult::createFailure(
			"Implicit surface smoothing produced non-finite geometry.");
	}
	mesh.calc_normals();
	const ImplicitFieldBounds generated_bounds = calculateMeshBounds(mesh);
	if (!generated_bounds.isValid()) {
		return ImplicitSurfaceGenerationResult::createFailure(
			"Implicit surface generation produced collapsed geometry.");
	}

	const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(mesh, {});
	return ImplicitSurfaceGenerationResult::createSuccess(
		GeneratedPrimitiveMesh(std::make_shared<Mesh>(std::move(mesh)), {}),
		generated_bounds,
		topology);
}
