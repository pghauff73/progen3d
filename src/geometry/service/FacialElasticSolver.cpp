#include "geometry/service/FacialElasticSolver.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {

using Positions = std::vector<glm::dvec3>;

bool finiteVector(const glm::dvec3 &value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool fixedVertex(const FacialElasticSystem &system, std::size_t index)
{
    return !system.fixed_vertices.empty() && system.fixed_vertices[index];
}

void require(bool condition, const char *message)
{
    if (!condition) throw std::invalid_argument(message);
}

void validateBendingStencil(const FacialElasticBendingStencil &stencil,
                            const Positions &rest_positions)
{
    for (std::size_t index : stencil.normal_vertex_indices)
        require(index < rest_positions.size(), "Facial bending normal index outside the position array.");
    const auto &indices = stencil.normal_vertex_indices;
    require(indices[0] != indices[1] && indices[1] != indices[2] && indices[2] != indices[0],
            "Facial bending normal triangle repeats a vertex.");
    require(std::isfinite(stencil.rest_area_m2) && stencil.rest_area_m2 > 0.0,
            "Facial bending stencil requires positive finite rest area.");
    require(std::isfinite(stencil.bending_rigidity_n_m) && stencil.bending_rigidity_n_m >= 0.0,
            "Facial bending rigidity must be finite and nonnegative.");
    require(std::isfinite(stencil.poisson_ratio) && stencil.poisson_ratio > -1.0 &&
            stencil.poisson_ratio < 0.5, "Facial bending Poisson ratio must be in (-1, 0.5).");
    require(finiteVector(stencil.rest_curvature_per_m), "Nonfinite facial rest curvature.");
    require(!stencil.curvature_weights.empty(), "Facial bending stencil requires curvature weights.");
    std::unordered_set<std::size_t> unique_vertices;
    for (const auto &weight : stencil.curvature_weights) {
        require(weight.vertex_index < rest_positions.size() && finiteVector(weight.weights_per_m2),
                "Invalid facial curvature reconstruction weight.");
        require(unique_vertices.insert(weight.vertex_index).second,
                "Facial curvature reconstruction repeats a vertex.");
    }
    const double twice_area = glm::length(glm::cross(rest_positions[indices[1]] - rest_positions[indices[0]],
                                                    rest_positions[indices[2]] - rest_positions[indices[0]]));
    require(std::isfinite(twice_area) && twice_area > 0.0, "Degenerate facial bending rest normal.");
}

struct FacialBendingStencilGeometry
{
    glm::dvec3 first_edge{0.0};
    glm::dvec3 second_edge{0.0};
    glm::dvec3 normal{0.0};
    double twice_area = 0.0;
    std::array<glm::dvec3, 3> quadratic_derivatives{};
    glm::dvec3 curvature_per_m{0.0};
};

bool evaluateBendingGeometry(const FacialElasticBendingStencil &stencil,
                             const Positions &positions, FacialBendingStencilGeometry &geometry)
{
    const auto &indices = stencil.normal_vertex_indices;
    const glm::dvec3 &anchor = positions[indices[0]];
    geometry.first_edge = positions[indices[1]] - anchor;
    geometry.second_edge = positions[indices[2]] - anchor;
    const glm::dvec3 cross = glm::cross(geometry.first_edge, geometry.second_edge);
    geometry.twice_area = glm::length(cross);
    if (!std::isfinite(geometry.twice_area) || geometry.twice_area <= 0.0) return false;
    geometry.normal = cross / geometry.twice_area;
    for (auto &derivative : geometry.quadratic_derivatives) derivative = glm::dvec3(0.0);
    for (const auto &weight : stencil.curvature_weights) {
        const glm::dvec3 offset = positions[weight.vertex_index] - anchor;
        for (int component = 0; component < 3; ++component)
            geometry.quadratic_derivatives[component] += weight.weights_per_m2[component] * offset;
    }
    for (int component = 0; component < 3; ++component)
        geometry.curvature_per_m[component] = glm::dot(geometry.normal,
                                                       geometry.quadratic_derivatives[component]);
    return finiteVector(geometry.curvature_per_m);
}

// Exact derivative of n(x) dot projection_vector, holding projection_vector
// constant. The normal comes only from the central triangle, so these three
// entries supplement the direct reconstruction derivatives at patch vertices.
std::array<glm::dvec3, 3> normalProjectionGradient(
    const FacialBendingStencilGeometry &geometry, const glm::dvec3 &projection_vector)
{
    const glm::dvec3 tangent = (projection_vector - geometry.normal *
        glm::dot(geometry.normal, projection_vector)) / geometry.twice_area;
    const glm::dvec3 second = glm::cross(geometry.second_edge, tangent);
    const glm::dvec3 third = glm::cross(tangent, geometry.first_edge);
    return {{-second - third, second, third}};
}

bool addBendingEnergy(const FacialElasticBendingStencil &stencil, const Positions &positions,
                      FacialElasticEnergyEvaluation &evaluation)
{
    if (stencil.bending_rigidity_n_m == 0.0) return true;
    FacialBendingStencilGeometry geometry;
    if (!evaluateBendingGeometry(stencil, positions, geometry)) return false;
    const glm::dvec3 change = geometry.curvature_per_m - stencil.rest_curvature_per_m;
    const double poisson = stencil.poisson_ratio;
    const double coefficient = stencil.rest_area_m2 * stencil.bending_rigidity_n_m;
    evaluation.bending_energy_j += 0.5 * coefficient *
        (change.x * change.x + change.y * change.y + 2.0 * poisson * change.x * change.y +
         2.0 * (1.0 - poisson) * change.z * change.z);
    const glm::dvec3 curvature_gradient = coefficient * glm::dvec3(
        change.x + poisson * change.y, change.y + poisson * change.x,
        2.0 * (1.0 - poisson) * change.z);
    double anchor_coefficient = 0.0;
    for (const auto &weight : stencil.curvature_weights) {
        const double contribution = glm::dot(weight.weights_per_m2, curvature_gradient);
        evaluation.gradient_n[weight.vertex_index] += contribution * geometry.normal;
        anchor_coefficient += contribution;
    }
    evaluation.gradient_n[stencil.normal_vertex_indices[0]] -= anchor_coefficient * geometry.normal;
    glm::dvec3 normal_projection(0.0);
    for (int component = 0; component < 3; ++component)
        normal_projection += curvature_gradient[component] * geometry.quadratic_derivatives[component];
    const auto normal_gradient = normalProjectionGradient(geometry, normal_projection);
    for (int corner = 0; corner < 3; ++corner)
        evaluation.gradient_n[stencil.normal_vertex_indices[corner]] += normal_gradient[corner];
    const double mean = 0.5 * (change.x + change.y);
    const double radius = std::hypot(0.5 * (change.x - change.y), change.z);
    evaluation.maximum_bending_curvature_change_per_m = std::max(
        evaluation.maximum_bending_curvature_change_per_m,
        std::max(std::fabs(mean + radius), std::fabs(mean - radius)));
    return true;
}

struct FacialBendingCurvatureDerivative
{
    std::size_t vertex_index = 0;
    // Columns correspond to x/y/z variations; rows are xx/yy/xy curvature.
    glm::dmat3 derivative_per_m2{0.0};
};

std::vector<FacialBendingCurvatureDerivative> bendingCurvatureDerivatives(
    const FacialElasticBendingStencil &stencil, const FacialBendingStencilGeometry &geometry)
{
    std::vector<FacialBendingCurvatureDerivative> derivatives;
    derivatives.reserve(stencil.curvature_weights.size() + 3);
    const auto derivativeForVertex = [&derivatives](std::size_t vertex) -> glm::dmat3 & {
        for (auto &entry : derivatives)
            if (entry.vertex_index == vertex) return entry.derivative_per_m2;
        derivatives.push_back({vertex, glm::dmat3(0.0)});
        return derivatives.back().derivative_per_m2;
    };
    glm::dvec3 anchor_weights(0.0);
    for (const auto &weight : stencil.curvature_weights) {
        derivativeForVertex(weight.vertex_index) += glm::outerProduct(weight.weights_per_m2, geometry.normal);
        anchor_weights += weight.weights_per_m2;
    }
    derivativeForVertex(stencil.normal_vertex_indices[0]) -= glm::outerProduct(anchor_weights, geometry.normal);
    for (int component = 0; component < 3; ++component) {
        const auto normal_gradient = normalProjectionGradient(geometry, geometry.quadratic_derivatives[component]);
        for (int corner = 0; corner < 3; ++corner) {
            auto &derivative = derivativeForVertex(stencil.normal_vertex_indices[corner]);
            for (int axis = 0; axis < 3; ++axis)
                derivative[axis][component] += normal_gradient[corner][axis];
        }
    }
    return derivatives;
}

void validateSystem(const FacialElasticSystem &system)
{
    const std::size_t count = system.rest_positions.size();
    require(count != 0, "Facial elastic system requires positions.");
    for (const auto &position : system.rest_positions)
        require(finiteVector(position), "Nonfinite facial elastic rest position.");
    require(system.fixed_vertices.empty() || system.fixed_vertices.size() == count,
            "Facial fixed mask must match the position count.");
    require(system.external_forces_n.empty() || system.external_forces_n.size() == count,
            "Facial external forces must match the position count.");
    for (const auto &force : system.external_forces_n)
        require(finiteVector(force), "Nonfinite facial external force.");
    require((system.support_stiffness_n_per_m.empty() && system.support_targets.empty()) ||
            (system.support_stiffness_n_per_m.size() == count && system.support_targets.size() == count),
            "Facial support stiffness and targets must both match the position count.");
    for (std::size_t index = 0; index < system.support_stiffness_n_per_m.size(); ++index) {
        require(std::isfinite(system.support_stiffness_n_per_m[index]) &&
                system.support_stiffness_n_per_m[index] >= 0.0,
                "Facial support stiffness must be finite and nonnegative.");
        require(finiteVector(system.support_targets[index]), "Nonfinite facial support target.");
    }
    for (const auto &triangle : system.triangles) {
        for (std::size_t index : triangle.vertex_indices)
            require(index < count, "Facial triangle index outside the position array.");
        require(triangle.vertex_indices[0] != triangle.vertex_indices[1] &&
                triangle.vertex_indices[1] != triangle.vertex_indices[2] &&
                triangle.vertex_indices[2] != triangle.vertex_indices[0],
                "Facial material triangle repeats a vertex.");
        require(std::isfinite(triangle.rest_area_m2) && triangle.rest_area_m2 > 0.0,
                "Facial material triangle requires positive finite rest area.");
        require(std::isfinite(triangle.young_modulus_pa) && triangle.young_modulus_pa > 0.0 &&
                std::isfinite(triangle.thickness_m) && triangle.thickness_m > 0.0,
                "Facial membrane modulus and thickness must be positive and finite.");
        require(std::isfinite(triangle.poisson_ratio) && triangle.poisson_ratio > -1.0 &&
                triangle.poisson_ratio < 0.5,
                "Facial Poisson ratio must be in (-1, 0.5).");
        for (int column = 0; column < 2; ++column)
            for (int row = 0; row < 2; ++row)
                require(std::isfinite(triangle.inverse_rest_matrix[column][row]),
                        "Nonfinite facial material frame.");
        const double determinant = glm::determinant(triangle.inverse_rest_matrix);
        require(std::isfinite(determinant) && determinant != 0.0,
                "Singular facial material frame.");
    }
    for (const auto &stencil : system.bending_stencils)
        validateBendingStencil(stencil, system.rest_positions);
    std::unordered_set<std::string> muscle_names;
    for (const auto &muscle : system.muscles) {
        require(!muscle.name.empty() && muscle_names.insert(muscle.name).second,
                "Facial muscle names must be nonempty and unique.");
        require(!muscle.insertions.empty(), "Facial muscle requires an insertion patch.");
        require(finiteVector(muscle.origin), "Nonfinite facial muscle origin.");
        require(std::isfinite(muscle.rest_length_m) && muscle.rest_length_m > 0.0,
                "Facial muscle rest length must be finite and positive.");
        require(std::isfinite(muscle.activation) && muscle.activation >= 0.0 && muscle.activation <= 1.0,
                "Facial muscle activation must be in [0, 1].");
        require(std::isfinite(muscle.maximum_shortening) && muscle.maximum_shortening >= 0.0 &&
                muscle.maximum_shortening < 1.0,
                "Facial muscle shortening must be in [0, 1).");
        require(std::isfinite(muscle.stiffness_n_per_m) && muscle.stiffness_n_per_m >= 0.0,
                "Facial muscle stiffness must be finite and nonnegative.");
        double total_weight = 0.0;
        for (const auto &insertion : muscle.insertions) {
            require(insertion.vertex_index < count && std::isfinite(insertion.weight) &&
                    insertion.weight >= 0.0, "Invalid facial muscle insertion weight.");
            total_weight += insertion.weight;
        }
        require(std::fabs(total_weight - 1.0) <= 1.0e-8,
                "Facial muscle insertion weights must sum to one.");
    }
}

double coordinateDot(const Positions &first, const Positions &second)
{
    long double sum = 0.0L;
    for (std::size_t index = 0; index < first.size(); ++index)
        for (int axis = 0; axis < 3; ++axis)
            sum += static_cast<long double>(first[index][axis]) * second[index][axis];
    return static_cast<double>(sum);
}

double coordinateNorm(const Positions &values)
{
    return std::sqrt(std::max(0.0, coordinateDot(values, values)));
}

void addScaled(Positions &destination, const Positions &source, double scale)
{
    for (std::size_t index = 0; index < destination.size(); ++index)
        destination[index] += scale * source[index];
}

Positions freeGradient(const FacialElasticSystem &system,
                       const FacialElasticEnergyEvaluation &evaluation)
{
    Positions result = evaluation.gradient_n;
    for (std::size_t index = 0; index < result.size(); ++index)
        if (fixedVertex(system, index)) result[index] = glm::dvec3(0.0);
    return result;
}

FacialElasticEnergyEvaluation evaluateEnergy(const FacialElasticSystem &system,
                                            const Positions &positions)
{
    FacialElasticEnergyEvaluation result;
    result.gradient_n.assign(positions.size(), glm::dvec3(0.0));
    result.support_forces_n.assign(positions.size(), glm::dvec3(0.0));
    for (const auto &triangle : system.triangles) {
        const auto &indices = triangle.vertex_indices;
        const glm::dvec3 edge_first = positions[indices[1]] - positions[indices[0]];
        const glm::dvec3 edge_second = positions[indices[2]] - positions[indices[0]];
        const auto &inverse = triangle.inverse_rest_matrix;
        const glm::dvec3 deformation_first = edge_first * inverse[0][0] + edge_second * inverse[0][1];
        const glm::dvec3 deformation_second = edge_first * inverse[1][0] + edge_second * inverse[1][1];
        const double strain_xx = 0.5 * (glm::dot(deformation_first, deformation_first) - 1.0);
        const double strain_yy = 0.5 * (glm::dot(deformation_second, deformation_second) - 1.0);
        const double strain_xy = 0.5 * glm::dot(deformation_first, deformation_second);
        const double poisson = triangle.poisson_ratio;
        const double weighted_modulus = triangle.rest_area_m2 * triangle.thickness_m *
            triangle.young_modulus_pa / (1.0 - poisson * poisson);
        result.membrane_energy_j += 0.5 * weighted_modulus *
            (strain_xx * strain_xx + strain_yy * strain_yy +
             2.0 * poisson * strain_xx * strain_yy + 2.0 * (1.0 - poisson) * strain_xy * strain_xy);
        const double stress_xx = weighted_modulus * (strain_xx + poisson * strain_yy);
        const double stress_yy = weighted_modulus * (strain_yy + poisson * strain_xx);
        const double stress_xy = weighted_modulus * (1.0 - poisson) * strain_xy;
        const glm::dvec3 piola_first = deformation_first * stress_xx + deformation_second * stress_xy;
        const glm::dvec3 piola_second = deformation_first * stress_xy + deformation_second * stress_yy;
        const glm::dvec3 gradient_first = piola_first * inverse[0][0] + piola_second * inverse[1][0];
        const glm::dvec3 gradient_second = piola_first * inverse[0][1] + piola_second * inverse[1][1];
        result.gradient_n[indices[1]] += gradient_first;
        result.gradient_n[indices[2]] += gradient_second;
        result.gradient_n[indices[0]] -= gradient_first + gradient_second;
        const double mean = 0.5 * (strain_xx + strain_yy);
        const double radius = std::hypot(0.5 * (strain_xx - strain_yy), strain_xy);
        result.maximum_green_strain = std::max(result.maximum_green_strain,
            std::max(std::fabs(mean + radius), std::fabs(mean - radius)));
    }
    for (std::size_t index = 0; index < system.support_stiffness_n_per_m.size(); ++index) {
        const glm::dvec3 displacement = positions[index] - system.support_targets[index];
        const double stiffness = system.support_stiffness_n_per_m[index];
        result.support_energy_j += 0.5 * stiffness * glm::dot(displacement, displacement);
        result.gradient_n[index] += stiffness * displacement;
        result.support_forces_n[index] = -stiffness * displacement;
    }
    for (const auto &muscle : system.muscles) {
        FacialElasticMuscleState state;
        state.name = muscle.name;
        for (const auto &insertion : muscle.insertions)
            state.insertion_position += insertion.weight * positions[insertion.vertex_index];
        const glm::dvec3 origin_to_insertion = state.insertion_position - muscle.origin;
        state.length_m = glm::length(origin_to_insertion);
        state.target_length_m = muscle.rest_length_m * (1.0 - muscle.activation * muscle.maximum_shortening);
        state.extension_m = std::max(0.0, state.length_m - state.target_length_m);
        state.tension_n = muscle.stiffness_n_per_m * state.extension_m;
        state.energy_j = 0.5 * muscle.stiffness_n_per_m * state.extension_m * state.extension_m;
        if (state.tension_n > 0.0) {
            const glm::dvec3 insertion_gradient = state.tension_n * origin_to_insertion / state.length_m;
            for (const auto &insertion : muscle.insertions)
                result.gradient_n[insertion.vertex_index] += insertion.weight * insertion_gradient;
        }
        result.muscle_energy_j += state.energy_j;
        result.muscle_lengths_m.push_back(state.length_m);
        result.muscle_tensions_n.push_back(state.tension_n);
        result.muscle_states.push_back(std::move(state));
    }
    for (std::size_t index = 0; index < system.external_forces_n.size(); ++index) {
        result.external_potential_j -= glm::dot(system.external_forces_n[index],
                                                positions[index] - system.rest_positions[index]);
        result.gradient_n[index] -= system.external_forces_n[index];
    }
    for (const auto &stencil : system.bending_stencils) {
        if (!addBendingEnergy(stencil, positions, result)) {
            // A collapsed trial normal is not a zero-curvature configuration.
            // The line search rejects this nonfinite trial without projection.
            result.bending_energy_j = std::numeric_limits<double>::infinity();
            result.total_energy_j = std::numeric_limits<double>::infinity();
            return result;
        }
    }
    result.total_energy_j = result.membrane_energy_j + result.support_energy_j +
                            result.muscle_energy_j + result.external_potential_j + result.bending_energy_j;
    return result;
}

bool finiteEvaluation(const FacialElasticEnergyEvaluation &evaluation)
{
    if (!std::isfinite(evaluation.total_energy_j) || !std::isfinite(evaluation.maximum_green_strain) ||
        !std::isfinite(evaluation.maximum_bending_curvature_change_per_m)) return false;
    return std::all_of(evaluation.gradient_n.begin(), evaluation.gradient_n.end(), finiteVector);
}

Positions stiffnessDiagonal(const FacialElasticSystem &system)
{
    Positions diagonal(system.rest_positions.size(), glm::dvec3(0.0));
    for (std::size_t index = 0; index < system.support_stiffness_n_per_m.size(); ++index)
        diagonal[index] += glm::dvec3(system.support_stiffness_n_per_m[index]);
    for (const auto &triangle : system.triangles) {
        const auto &indices = triangle.vertex_indices;
        const auto &inverse = triangle.inverse_rest_matrix;
        const glm::dvec3 edge_first = system.rest_positions[indices[1]] - system.rest_positions[indices[0]];
        const glm::dvec3 edge_second = system.rest_positions[indices[2]] - system.rest_positions[indices[0]];
        const glm::dvec3 material_first = edge_first * inverse[0][0] + edge_second * inverse[0][1];
        const glm::dvec3 material_second = edge_first * inverse[1][0] + edge_second * inverse[1][1];
        const glm::dvec2 gradients[] = {
            {-inverse[0][0] - inverse[0][1], -inverse[1][0] - inverse[1][1]},
            {inverse[0][0], inverse[1][0]},
            {inverse[0][1], inverse[1][1]}};
        const double poisson = triangle.poisson_ratio;
        const double coefficient = triangle.rest_area_m2 * triangle.thickness_m *
            triangle.young_modulus_pa / (1.0 - poisson * poisson);
        for (int corner = 0; corner < 3; ++corner) {
            for (int axis = 0; axis < 3; ++axis) {
                const double derivative_xx = material_first[axis] * gradients[corner].x;
                const double derivative_yy = material_second[axis] * gradients[corner].y;
                const double derivative_xy = 0.5 * (material_second[axis] * gradients[corner].x +
                                                    material_first[axis] * gradients[corner].y);
                diagonal[indices[corner]][axis] += coefficient *
                    (derivative_xx * derivative_xx + derivative_yy * derivative_yy +
                     2.0 * poisson * derivative_xx * derivative_yy +
                     2.0 * (1.0 - poisson) * derivative_xy * derivative_xy);
            }
        }
    }
    for (const auto &muscle : system.muscles)
        for (const auto &insertion : muscle.insertions)
            diagonal[insertion.vertex_index] += glm::dvec3(
                muscle.stiffness_n_per_m * insertion.weight * insertion.weight);
    for (const auto &stencil : system.bending_stencils) {
        if (stencil.bending_rigidity_n_m == 0.0) continue;
        FacialBendingStencilGeometry geometry;
        if (!evaluateBendingGeometry(stencil, system.rest_positions, geometry))
            throw std::invalid_argument("Invalid facial bending rest reconstruction.");
        const double coefficient = stencil.rest_area_m2 * stencil.bending_rigidity_n_m;
        const double poisson = stencil.poisson_ratio;
        // J^T C J is the exact bending Hessian at its neutral state and a
        // positive semidefinite metric elsewhere. It changes only numerical
        // preconditioning, never the objective or reported force residual.
        for (const auto &entry : bendingCurvatureDerivatives(stencil, geometry)) {
            for (int axis = 0; axis < 3; ++axis) {
                const glm::dvec3 derivative = entry.derivative_per_m2[axis];
                diagonal[entry.vertex_index][axis] += coefficient *
                    (derivative.x * derivative.x + derivative.y * derivative.y +
                     2.0 * poisson * derivative.x * derivative.y +
                     2.0 * (1.0 - poisson) * derivative.z * derivative.z);
            }
        }
    }
    return diagonal;
}

double characteristicLength(const FacialElasticSystem &system)
{
    glm::dvec3 minimum = system.rest_positions.front();
    glm::dvec3 maximum = minimum;
    for (const auto &position : system.rest_positions) {
        minimum = glm::min(minimum, position);
        maximum = glm::max(maximum, position);
    }
    double length = glm::length(maximum - minimum);
    for (const auto &muscle : system.muscles) length = std::max(length, muscle.rest_length_m);
    for (std::size_t index = 0; index < system.support_targets.size(); ++index)
        length = std::max(length, glm::length(system.support_targets[index] - system.rest_positions[index]));
    return length > 0.0 ? length : 1.0;
}

struct FacialElasticCurvaturePair
{
    Positions position_difference;
    Positions gradient_difference;
    double inverse_curvature = 0.0;
};

Positions descentDirection(const Positions &gradient, const Positions &inverse_diagonal,
                           const std::deque<FacialElasticCurvaturePair> &history)
{
    Positions reduced_gradient = gradient;
    std::vector<double> coefficients(history.size(), 0.0);
    for (std::size_t reverse = history.size(); reverse != 0; --reverse) {
        const std::size_t index = reverse - 1;
        coefficients[index] = history[index].inverse_curvature *
            coordinateDot(history[index].position_difference, reduced_gradient);
        addScaled(reduced_gradient, history[index].gradient_difference, -coefficients[index]);
    }
    double scale = 1.0;
    if (!history.empty()) {
        const auto &latest = history.back();
        Positions scaled_gradient = latest.gradient_difference;
        for (std::size_t index = 0; index < scaled_gradient.size(); ++index)
            scaled_gradient[index] *= inverse_diagonal[index];
        const double denominator = coordinateDot(latest.gradient_difference, scaled_gradient);
        if (denominator > 0.0)
            scale = std::clamp(1.0 / (latest.inverse_curvature * denominator), 1.0e-4, 1.0e4);
    }
    Positions direction = reduced_gradient;
    for (std::size_t index = 0; index < direction.size(); ++index)
        direction[index] *= scale * inverse_diagonal[index];
    for (std::size_t index = 0; index < history.size(); ++index) {
        const double coefficient = history[index].inverse_curvature *
            coordinateDot(history[index].gradient_difference, direction);
        addScaled(direction, history[index].position_difference, coefficients[index] - coefficient);
    }
    for (auto &value : direction) value = -value;
    return direction;
}

void updateResidual(FacialElasticSolveResult &result, const Positions &gradient)
{
    result.free_force_l2_n = coordinateNorm(gradient);
    result.relative_free_force = result.force_scale_n > 0.0 ?
        result.free_force_l2_n / result.force_scale_n :
        (result.free_force_l2_n == 0.0 ? 0.0 : std::numeric_limits<double>::infinity());
}

} // namespace

FacialElasticTriangle FacialElasticSolver::makeRestTriangle(
    std::array<std::size_t, 3> vertex_indices, const Positions &rest_positions,
    double young_modulus_pa, double poisson_ratio, double thickness_m)
{
    for (std::size_t index : vertex_indices)
        require(index < rest_positions.size(), "Facial rest triangle index outside the position array.");
    const glm::dvec3 edge_first = rest_positions[vertex_indices[1]] - rest_positions[vertex_indices[0]];
    const glm::dvec3 edge_second = rest_positions[vertex_indices[2]] - rest_positions[vertex_indices[0]];
    const double first_length = glm::length(edge_first);
    const double twice_area = glm::length(glm::cross(edge_first, edge_second));
    require(std::isfinite(first_length) && first_length > 0.0 &&
            std::isfinite(twice_area) && twice_area > 0.0, "Degenerate facial rest triangle.");
    const double horizontal_second = glm::dot(edge_first, edge_second) / first_length;
    const double vertical_second = twice_area / first_length;
    const glm::dmat2 rest_matrix(glm::dvec2(first_length, 0.0),
                                 glm::dvec2(horizontal_second, vertical_second));
    FacialElasticTriangle result;
    result.vertex_indices = vertex_indices;
    result.inverse_rest_matrix = glm::inverse(rest_matrix);
    result.rest_area_m2 = 0.5 * twice_area;
    result.young_modulus_pa = young_modulus_pa;
    result.poisson_ratio = poisson_ratio;
    result.thickness_m = thickness_m;
    return result;
}

FacialElasticBendingStencil FacialElasticSolver::makeRestBendingStencil(
    std::array<std::size_t, 3> normal_vertex_indices,
    std::vector<FacialElasticCurvatureWeight> curvature_weights,
    const Positions &rest_positions, double bending_rigidity_n_m, double poisson_ratio)
{
    for (std::size_t index : normal_vertex_indices)
        require(index < rest_positions.size(), "Facial bending normal index outside the position array.");
    FacialElasticBendingStencil result;
    result.normal_vertex_indices = normal_vertex_indices;
    result.curvature_weights = std::move(curvature_weights);
    result.rest_area_m2 = 0.5 * glm::length(glm::cross(
        rest_positions[normal_vertex_indices[1]] - rest_positions[normal_vertex_indices[0]],
        rest_positions[normal_vertex_indices[2]] - rest_positions[normal_vertex_indices[0]]));
    result.bending_rigidity_n_m = bending_rigidity_n_m;
    result.poisson_ratio = poisson_ratio;
    validateBendingStencil(result, rest_positions);
    FacialBendingStencilGeometry geometry;
    require(evaluateBendingGeometry(result, rest_positions, geometry),
            "Invalid facial bending rest reconstruction.");
    result.rest_curvature_per_m = geometry.curvature_per_m;
    return result;
}

FacialElasticEnergyEvaluation FacialElasticSolver::evaluate(
    const FacialElasticSystem &system, const Positions &positions) const
{
    validateSystem(system);
    require(positions.size() == system.rest_positions.size(), "Facial evaluation position count mismatch.");
    for (const auto &position : positions)
        require(finiteVector(position), "Nonfinite facial evaluation position.");
    auto result = evaluateEnergy(system, positions);
    if (!finiteEvaluation(result)) throw std::overflow_error("Nonfinite facial elastic energy or gradient.");
    return result;
}

FacialElasticSolveResult FacialElasticSolver::solve(
    const FacialElasticSystem &system, const FacialElasticSolverSettings &settings,
    AdmissibilityCallback admissibility) const
{
    validateSystem(system);
    require(settings.max_iterations >= 1 && settings.max_iterations <= 2000,
            "Facial elastic iteration budget must be in [1, 2000].");
    require(std::isfinite(settings.relative_force_tolerance) &&
            settings.relative_force_tolerance > 0.0 && settings.relative_force_tolerance < 1.0,
            "Facial relative force tolerance must be in (0, 1).");
    require(settings.history_size <= 32 && settings.max_line_search_steps >= 1 &&
            settings.max_line_search_steps <= 64, "Invalid facial elastic line search or history settings.");
    FacialElasticSolveResult result;
    result.positions = system.rest_positions;
    result.final_evaluation = evaluateEnergy(system, result.positions);
    if (!finiteEvaluation(result.final_evaluation))
        throw std::overflow_error("Nonfinite initial facial elastic energy or gradient.");
    result.accepted_energies_j.push_back(result.final_evaluation.total_energy_j);
    Positions gradient = freeGradient(system, result.final_evaluation);
    const Positions diagonal = stiffnessDiagonal(system);
    double largest_stiffness = 0.0;
    std::size_t free_degrees = 0;
    for (std::size_t index = 0; index < diagonal.size(); ++index) {
        if (fixedVertex(system, index)) continue;
        free_degrees += 3;
        for (int axis = 0; axis < 3; ++axis)
            largest_stiffness = std::max(largest_stiffness, diagonal[index][axis]);
    }
    // The numerical floor scales with every elastic coefficient. It prevents
    // neutral roundoff from becoming a relative residual of one, without adding
    // any force, energy, inertia or absolute stiffness to the mechanical model.
    const double numerical_force_floor = largest_stiffness * characteristicLength(system) *
        std::sqrt(static_cast<double>(free_degrees)) * 1.0e-8;
    result.force_scale_n = std::max(coordinateNorm(gradient), numerical_force_floor);
    updateResidual(result, gradient);
    if (admissibility && !admissibility(result.positions)) {
        result.admissible = false;
        result.termination_reason = "initial geometry is inadmissible";
        return result;
    }
    Positions inverse_diagonal(diagonal.size(), glm::dvec3(0.0));
    const double diagonal_floor = largest_stiffness > 0.0 ? largest_stiffness * 1.0e-8 : 1.0;
    for (std::size_t index = 0; index < diagonal.size(); ++index) {
        if (fixedVertex(system, index)) continue;
        for (int axis = 0; axis < 3; ++axis)
            inverse_diagonal[index][axis] = 1.0 / std::max(diagonal[index][axis], diagonal_floor);
    }
    std::deque<FacialElasticCurvaturePair> history;
    for (std::size_t iteration = 0; iteration < settings.max_iterations; ++iteration) {
        if (result.relative_free_force <= settings.relative_force_tolerance) {
            result.converged = true;
            result.termination_reason = "free-force tolerance satisfied";
            return result;
        }
        Positions direction = descentDirection(gradient, inverse_diagonal, history);
        double directional_derivative = coordinateDot(gradient, direction);
        if (!std::isfinite(directional_derivative) || directional_derivative >= 0.0) {
            history.clear();
            direction = descentDirection(gradient, inverse_diagonal, history);
            directional_derivative = coordinateDot(gradient, direction);
        }
        bool accepted = false;
        bool geometry_rejected = false;
        Positions trial;
        FacialElasticEnergyEvaluation trial_evaluation;
        // A stale curvature model may fail near changing cable activity. Retry
        // once with the mechanical diagonal, without changing the objective.
        for (int attempt = 0; attempt < 2 && !accepted; ++attempt) {
            if (attempt != 0) {
                if (history.empty()) break;
                history.clear();
                direction = descentDirection(gradient, inverse_diagonal, history);
                directional_derivative = coordinateDot(gradient, direction);
            }
            if (!std::isfinite(directional_derivative) || directional_derivative >= 0.0) break;
            double step = 1.0;
            for (std::size_t backtrack = 0; backtrack < settings.max_line_search_steps; ++backtrack, step *= 0.5) {
                trial = result.positions;
                addScaled(trial, direction, step);
                if (!std::all_of(trial.begin(), trial.end(), finiteVector)) continue;
                trial_evaluation = evaluateEnergy(system, trial);
                if (!finiteEvaluation(trial_evaluation) ||
                    trial_evaluation.total_energy_j > result.final_evaluation.total_energy_j +
                        1.0e-4 * step * directional_derivative) continue;
                if (admissibility && !admissibility(trial)) {
                    geometry_rejected = true;
                    continue;
                }
                accepted = true;
                break;
            }
        }
        if (!accepted) {
            result.termination_reason = geometry_rejected ?
                "line search blocked by geometry admissibility" : "line search could not reduce energy";
            return result;
        }
        Positions trial_gradient = freeGradient(system, trial_evaluation);
        FacialElasticCurvaturePair curvature;
        curvature.position_difference = trial;
        curvature.gradient_difference = trial_gradient;
        addScaled(curvature.position_difference, result.positions, -1.0);
        addScaled(curvature.gradient_difference, gradient, -1.0);
        const double product = coordinateDot(curvature.position_difference, curvature.gradient_difference);
        const double norm_product = coordinateNorm(curvature.position_difference) *
                                    coordinateNorm(curvature.gradient_difference);
        if (settings.history_size != 0 && std::isfinite(product) && product > 1.0e-10 * norm_product) {
            curvature.inverse_curvature = 1.0 / product;
            history.push_back(std::move(curvature));
            if (history.size() > settings.history_size) history.pop_front();
        }
        result.positions = std::move(trial);
        result.final_evaluation = std::move(trial_evaluation);
        gradient = std::move(trial_gradient);
        result.iterations = iteration + 1;
        result.accepted_energies_j.push_back(result.final_evaluation.total_energy_j);
        updateResidual(result, gradient);
    }
    result.converged = result.relative_free_force <= settings.relative_force_tolerance;
    result.termination_reason = result.converged ?
        "free-force tolerance satisfied" : "iteration budget exhausted";
    return result;
}
