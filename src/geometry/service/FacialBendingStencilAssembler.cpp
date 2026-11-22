#include "geometry/service/FacialBendingStencilAssembler.h"

#include "geometry/service/FacialElasticSolver.h"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct FacialCurvaturePatchReconstruction
{
	std::vector<FacialElasticCurvatureWeight> weights;
	double condition = std::numeric_limits<double>::infinity();
	double polynomial_error = std::numeric_limits<double>::infinity();
	double height_ratio = 0.0;
	bool valid = false;
};

class FacialRestCurvatureReconstruction
{
public:
	FacialCurvaturePatchReconstruction reconstruct(
		const std::vector<glm::dvec3> &positions,
		const std::array<std::size_t, 3> &triangle,
		const std::set<std::size_t> &patch) const
	{
		FacialCurvaturePatchReconstruction result;
		if (patch.size() < 6) return result;
		const glm::dvec3 origin = (positions[triangle[0]] + positions[triangle[1]] +
			positions[triangle[2]]) / 3.0;
		const glm::dvec3 first_edge = positions[triangle[1]] - positions[triangle[0]];
		const glm::dvec3 cross = glm::cross(first_edge, positions[triangle[2]] - positions[triangle[0]]);
		const double cross_length = glm::length(cross);
		if (!(cross_length > 0.0) || !std::isfinite(cross_length)) return result;
		const glm::dvec3 normal = cross / cross_length;
		const glm::dvec3 horizontal = glm::normalize(first_edge);
		const glm::dvec3 vertical = glm::cross(normal, horizontal);
		std::vector<glm::dvec2> coordinates;
		double coordinate_scale = 0.0, maximum_height = 0.0;
		for (std::size_t vertex : patch) {
			const glm::dvec3 offset = positions[vertex] - origin;
			const glm::dvec2 coordinate(glm::dot(offset, horizontal), glm::dot(offset, vertical));
			coordinates.push_back(coordinate);
			coordinate_scale = std::max(coordinate_scale, glm::length(coordinate));
			maximum_height = std::max(maximum_height, std::fabs(glm::dot(offset, normal)));
		}
		if (!(coordinate_scale > 0.0) || !std::isfinite(coordinate_scale)) return result;
		result.height_ratio = maximum_height / coordinate_scale;
		Eigen::MatrixXd polynomial(static_cast<Eigen::Index>(patch.size()), 6);
		for (std::size_t index = 0; index < coordinates.size(); ++index) {
			const double x = coordinates[index].x / coordinate_scale;
			const double y = coordinates[index].y / coordinate_scale;
			polynomial.row(static_cast<Eigen::Index>(index)) << 1.0, x, y, 0.5*x*x, 0.5*y*y, x*y;
		}
		// Normalized coordinates keep the condition criterion independent of the
		// face's unit scale. SVD diagnoses deficient/ill-conditioned patches; a
		// larger connected patch is tried explicitly, with no omitted free seam.
		Eigen::JacobiSVD<Eigen::MatrixXd> decomposition(polynomial, Eigen::ComputeThinU | Eigen::ComputeThinV);
		const auto singular = decomposition.singularValues();
		if (singular.size() != 6 || !(singular[5] > 0.0)) return result;
		result.condition = singular[0] / singular[5];
		if (!std::isfinite(result.condition) || result.condition > 1.0e5) return result;
		const Eigen::MatrixXd inverse = decomposition.solve(
			Eigen::MatrixXd::Identity(polynomial.rows(), polynomial.rows()));
		const Eigen::MatrixXd reproduction = inverse * polynomial;
		result.polynomial_error = 0.0;
		for (int derivative = 3; derivative < 6; ++derivative) {
			for (int basis = 0; basis < 6; ++basis) {
				const double target = derivative == basis ? 1.0 : 0.0;
				result.polynomial_error = std::max(result.polynomial_error,
					std::fabs(reproduction(derivative, basis) - target));
			}
		}
		if (!std::isfinite(result.polynomial_error) || result.polynomial_error > 1.0e-8) return result;
		const double derivative_scale = 1.0 / (coordinate_scale * coordinate_scale);
		std::size_t index = 0;
		for (std::size_t vertex : patch) {
			const Eigen::Index column = static_cast<Eigen::Index>(index++);
			result.weights.push_back({vertex, derivative_scale *
				glm::dvec3(inverse(3, column), inverse(4, column), inverse(5, column))});
		}
		result.valid = true;
		return result;
	}
};

bool allFixed(const FacialElasticSystem &system, const std::set<std::size_t> &vertices)
{
	if (system.fixed_vertices.empty()) return false;
	return std::all_of(vertices.begin(), vertices.end(), [&](std::size_t index) {
		return system.fixed_vertices[index];
	});
}

double maximumPrincipalMagnitude(const glm::dvec3 &curvature)
{
	const double mean = 0.5 * (curvature.x + curvature.y);
	const double radius = std::hypot(0.5 * (curvature.x - curvature.y), curvature.z);
	return std::max(std::fabs(mean + radius), std::fabs(mean - radius));
}

} // namespace

FacialBendingAssemblyDiagnostics FacialBendingStencilAssembler::assemble(
	FacialElasticSystem &system, double stiffness_scale) const
{
	FacialBendingAssemblyDiagnostics diagnostics;
	system.bending_stencils.clear();
	if (!std::isfinite(stiffness_scale) || stiffness_scale < 0.0)
		throw std::invalid_argument("Facial bending scale must be finite and nonnegative.");
	if (stiffness_scale == 0.0) return diagnostics;
	if (!system.fixed_vertices.empty() && system.fixed_vertices.size() != system.rest_positions.size())
		throw std::invalid_argument("Facial bending fixed mask must match the material vertices.");
	std::vector<std::set<std::size_t>> adjacency(system.rest_positions.size());
	for (const auto &triangle : system.triangles) {
		for (std::size_t vertex : triangle.vertex_indices)
			if (vertex >= system.rest_positions.size()) throw std::invalid_argument("Facial bending triangle index is invalid.");
		for (int corner = 0; corner < 3; ++corner) {
			const auto first = triangle.vertex_indices[corner], second = triangle.vertex_indices[(corner + 1) % 3];
			adjacency[first].insert(second); adjacency[second].insert(first);
		}
	}
	const FacialRestCurvatureReconstruction reconstruction;
	for (std::size_t face_index = 0; face_index < system.triangles.size(); ++face_index) {
		const auto &triangle = system.triangles[face_index];
		std::set<std::size_t> patch(triangle.vertex_indices.begin(), triangle.vertex_indices.end());
		FacialCurvaturePatchReconstruction fit;
		std::size_t ring = 0;
		for (ring = 1; ring <= 3; ++ring) {
			const auto previous = patch;
			for (std::size_t vertex : previous) patch.insert(adjacency[vertex].begin(), adjacency[vertex].end());
			fit = reconstruction.reconstruct(system.rest_positions, triangle.vertex_indices, patch);
			if (fit.valid) break;
		}
		if (!fit.valid) {
			// Search geometry independently of constraints. Even a wholly fixed
			// first ring may need free vertices from a larger ring to reconstruct
			// curvature and transmit moment across a fixed/free boundary.
			if (allFixed(system, patch)) {
				++diagnostics.fixed_only_triangles;
				++diagnostics.fixed_only_unreconstructable_triangles;
				continue;
			}
			std::ostringstream message;
			message << "Facial bending curvature reconstruction failed at triangle " << face_index
			        << "; connected patch vertices=" << patch.size()
			        << "; condition=" << fit.condition
			        << "; polynomial reproduction error=" << fit.polynomial_error;
			throw std::runtime_error(message.str());
		}
		// Only the full, geometry-selected valid stencil can be proved to have
		// identically zero rest-relative energy when all of its vertices are fixed.
		if (allFixed(system, patch)) { ++diagnostics.fixed_only_triangles; continue; }
		const double rigidity = stiffness_scale * triangle.young_modulus_pa *
			triangle.thickness_m * triangle.thickness_m * triangle.thickness_m /
			(12.0 * (1.0 - triangle.poisson_ratio * triangle.poisson_ratio));
		auto stencil = FacialElasticSolver::makeRestBendingStencil(triangle.vertex_indices,
			std::move(fit.weights), system.rest_positions, rigidity, triangle.poisson_ratio);
		++diagnostics.active_stencils;
		if (ring > 1) ++diagnostics.expanded_stencils;
		diagnostics.maximum_patch_vertices = std::max(diagnostics.maximum_patch_vertices, patch.size());
		diagnostics.maximum_neighborhood_rings = std::max(diagnostics.maximum_neighborhood_rings, ring);
		diagnostics.maximum_reconstruction_condition = std::max(diagnostics.maximum_reconstruction_condition, fit.condition);
		diagnostics.maximum_polynomial_reproduction_error = std::max(diagnostics.maximum_polynomial_reproduction_error, fit.polynomial_error);
		diagnostics.maximum_patch_height_ratio = std::max(diagnostics.maximum_patch_height_ratio, fit.height_ratio);
		diagnostics.maximum_rest_curvature_per_m = std::max(diagnostics.maximum_rest_curvature_per_m,
			maximumPrincipalMagnitude(stencil.rest_curvature_per_m));
		system.bending_stencils.push_back(std::move(stencil));
	}
	return diagnostics;
}
