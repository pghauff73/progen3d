#include "geometry/service/FacialElasticSolver.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void check(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkNear(double actual, double expected, double absolute_tolerance,
               double relative_tolerance, const std::string &message)
{
    check(std::isfinite(actual) && std::fabs(actual - expected) <= absolute_tolerance +
          relative_tolerance * std::max(std::fabs(actual), std::fabs(expected)), message);
}

void checkVector(const glm::dvec3 &actual, const glm::dvec3 &expected,
                 double tolerance, const std::string &message)
{
    check(std::isfinite(glm::length(actual)) && glm::length(actual - expected) <= tolerance, message);
}

void checkMonotone(const FacialElasticSolveResult &result)
{
    check(result.accepted_energies_j.size() == result.iterations + 1, "Incomplete accepted energy history.");
    for (std::size_t index = 1; index < result.accepted_energies_j.size(); ++index)
        check(result.accepted_energies_j[index] <= result.accepted_energies_j[index - 1],
              "Accepted shell energy increased.");
}

double rigidity(double young, double thickness, double poisson)
{
    return young * thickness * thickness * thickness / (12.0 * (1.0 - poisson * poisson));
}

// Independent small test-fixture reconstruction. Production assembly uses SVD;
// these normalized, regular patches use a pivoted six-column normal-equation
// inverse. No production assembly code is used by the cylinder oracle.
bool invertSix(const std::array<std::array<double, 6>, 6> &matrix,
               std::array<std::array<double, 6>, 6> &inverse)
{
    std::array<std::array<double, 12>, 6> augmented{};
    double scale = 0.0;
    for (int row = 0; row < 6; ++row) {
        for (int column = 0; column < 6; ++column) {
            augmented[row][column] = matrix[row][column];
            scale = std::max(scale, std::fabs(matrix[row][column]));
        }
        augmented[row][row + 6] = 1.0;
    }
    for (int column = 0; column < 6; ++column) {
        int pivot = column;
        for (int row = column + 1; row < 6; ++row)
            if (std::fabs(augmented[row][column]) > std::fabs(augmented[pivot][column])) pivot = row;
        if (std::fabs(augmented[pivot][column]) < 1.0e-12 * scale) return false;
        std::swap(augmented[pivot], augmented[column]);
        const double divisor = augmented[column][column];
        for (int entry = 0; entry < 12; ++entry) augmented[column][entry] /= divisor;
        for (int row = 0; row < 6; ++row) {
            if (row == column) continue;
            const double factor = augmented[row][column];
            for (int entry = 0; entry < 12; ++entry)
                augmented[row][entry] -= factor * augmented[column][entry];
        }
    }
    for (int row = 0; row < 6; ++row)
        for (int column = 0; column < 6; ++column) inverse[row][column] = augmented[row][column + 6];
    return true;
}

class FacialBendingSheetFixture
{
public:
    FacialBendingSheetFixture(int horizontal_segments, int vertical_segments,
                              double length, double width, bool checkerboard,
                              double young, double poisson, double thickness,
                              bool include_membrane = false)
        : horizontal_segments_(horizontal_segments), vertical_segments_(vertical_segments)
    {
        for (int row = 0; row <= vertical_segments; ++row)
            for (int column = 0; column <= horizontal_segments; ++column)
                system.rest_positions.emplace_back(length * column / horizontal_segments,
                                                    width * row / vertical_segments, 0.0);
        adjacency_.resize(system.rest_positions.size());
        for (int row = 0; row < vertical_segments; ++row) {
            for (int column = 0; column < horizontal_segments; ++column) {
                const std::size_t first = index(column, row);
                const std::size_t second = index(column + 1, row);
                const std::size_t third = index(column, row + 1);
                const std::size_t fourth = index(column + 1, row + 1);
                if (checkerboard && (row + column) % 2) {
                    faces_.push_back({first, second, third});
                    faces_.push_back({second, fourth, third});
                } else {
                    faces_.push_back({first, second, fourth});
                    faces_.push_back({first, fourth, third});
                }
            }
        }
        for (const auto &face : faces_)
            for (std::size_t vertex : face)
                adjacency_[vertex].insert(face.begin(), face.end());
        for (const auto &face : faces_) {
            if (include_membrane)
                system.triangles.push_back(FacialElasticSolver::makeRestTriangle(
                    face, system.rest_positions, young, poisson, thickness));
            system.bending_stencils.push_back(FacialElasticSolver::makeRestBendingStencil(
                face, reconstruction(face, std::sqrt(length * width / horizontal_segments / vertical_segments)),
                system.rest_positions, rigidity(young, thickness, poisson), poisson));
        }
    }

    std::size_t index(int column, int row) const
    {
        return static_cast<std::size_t>(row * (horizontal_segments_ + 1) + column);
    }

    std::vector<glm::dvec3> cylinder(double curvature, double angle) const
    {
        auto positions = system.rest_positions;
        const glm::dvec2 direction(std::cos(angle), std::sin(angle));
        const glm::dvec2 axis(-direction.y, direction.x);
        for (auto &position : positions) {
            const glm::dvec2 flat(position);
            const double along = glm::dot(flat, direction);
            const double across = glm::dot(flat, axis);
            position = {std::sin(curvature * along) / curvature, across,
                        (1.0 - std::cos(curvature * along)) / curvature};
        }
        return positions;
    }

    FacialElasticSystem system;

private:
    std::vector<FacialElasticCurvatureWeight> reconstruction(
        const std::array<std::size_t, 3> &face, double length_scale) const
    {
        std::set<std::size_t> patch(face.begin(), face.end());
        for (int ring = 0; ring < 3; ++ring) {
            const auto previous = patch;
            for (std::size_t vertex : previous)
                patch.insert(adjacency_[vertex].begin(), adjacency_[vertex].end());
            std::vector<std::array<double, 6>> rows;
            std::array<std::array<double, 6>, 6> gram{};
            for (std::size_t vertex : patch) {
                const glm::dvec3 relative = (system.rest_positions[vertex] - system.rest_positions[face[0]]) / length_scale;
                const std::array<double, 6> row{{1.0, relative.x, relative.y,
                    0.5 * relative.x * relative.x, 0.5 * relative.y * relative.y, relative.x * relative.y}};
                rows.push_back(row);
                for (int first = 0; first < 6; ++first)
                    for (int second = 0; second < 6; ++second) gram[first][second] += row[first] * row[second];
            }
            std::array<std::array<double, 6>, 6> inverse{};
            if (!invertSix(gram, inverse)) continue;
            std::vector<FacialElasticCurvatureWeight> weights;
            std::size_t row_index = 0;
            for (std::size_t vertex : patch) {
                glm::dvec3 weight(0.0);
                for (int component = 0; component < 3; ++component)
                    for (int column = 0; column < 6; ++column)
                        weight[component] += inverse[component + 3][column] * rows[row_index][column] /
                                             (length_scale * length_scale);
                weights.push_back({vertex, weight});
                ++row_index;
            }
            return weights;
        }
        throw std::runtime_error("Independent sheet fixture could not reconstruct a quadratic patch.");
    }

    int horizontal_segments_;
    int vertical_segments_;
    std::vector<std::array<std::size_t, 3>> faces_;
    std::vector<std::set<std::size_t>> adjacency_;
};

FacialElasticSystem curvedStencil()
{
    FacialElasticSystem system;
    constexpr double spacing = 0.01;
    for (int row = -1; row <= 1; ++row) {
        for (int column = -1; column <= 1; ++column) {
            const double x = column * spacing, y = row * spacing;
            system.rest_positions.emplace_back(x, y, 0.5 * (3.0 * x * x - 2.0 * y * y) + 0.7 * x * y);
        }
    }
    std::vector<FacialElasticCurvatureWeight> weights;
    for (std::size_t index = 0; index < 9; ++index) weights.push_back({index, glm::dvec3(0.0)});
    const double inverse_square = 1.0 / (spacing * spacing);
    weights[3].weights_per_m2.x = inverse_square;
    weights[5].weights_per_m2.x = inverse_square;
    weights[4].weights_per_m2.x = -2.0 * inverse_square;
    weights[1].weights_per_m2.y = inverse_square;
    weights[7].weights_per_m2.y = inverse_square;
    weights[4].weights_per_m2.y = -2.0 * inverse_square;
    weights[0].weights_per_m2.z = 0.25 * inverse_square;
    weights[8].weights_per_m2.z = 0.25 * inverse_square;
    weights[2].weights_per_m2.z = -0.25 * inverse_square;
    weights[6].weights_per_m2.z = -0.25 * inverse_square;
    system.bending_stencils.push_back(FacialElasticSolver::makeRestBendingStencil(
        {4, 5, 7}, weights, system.rest_positions, 0.004, 0.3));
    return system;
}

std::vector<glm::dvec3> deformedStencil(const FacialElasticSystem &system)
{
    auto positions = system.rest_positions;
    for (std::size_t index = 0; index < positions.size(); ++index) {
        const double number = static_cast<double>(index + 1);
        positions[index] += glm::dvec3(0.00013 * std::sin(number),
                                       0.00009 * std::cos(1.7 * number),
                                       0.00017 * std::sin(0.9 * number));
    }
    return positions;
}

void testExactBendingGradient()
{
    const auto system = curvedStencil();
    const auto positions = deformedStencil(system);
    const FacialElasticSolver solver;
    const auto evaluation = solver.evaluate(system, positions);
    check(evaluation.bending_energy_j > 0.0 && evaluation.maximum_bending_curvature_change_per_m > 0.0,
          "Nontrivial curvature changes must create energy and a diagnostic.");
    double maximum_error = 0.0;
    for (std::size_t index = 0; index < positions.size(); ++index) {
        for (int axis = 0; axis < 3; ++axis) {
            auto positive = positions, negative = positions;
            constexpr double step = 1.0e-8;
            positive[index][axis] += step;
            negative[index][axis] -= step;
            const double finite_difference = (solver.evaluate(system, positive).total_energy_j -
                                               solver.evaluate(system, negative).total_energy_j) / (2.0 * step);
            maximum_error = std::max(maximum_error, std::fabs(finite_difference - evaluation.gradient_n[index][axis]));
            checkNear(evaluation.gradient_n[index][axis], finite_difference, 2.0e-9, 2.0e-6,
                      "Bending gradient must include the derivative of the current normal.");
        }
    }
    glm::dvec3 net_force(0.0), net_torque(0.0);
    for (std::size_t index = 0; index < positions.size(); ++index) {
        net_force += evaluation.gradient_n[index];
        net_torque += glm::cross(positions[index], evaluation.gradient_n[index]);
    }
    checkVector(net_force, glm::dvec3(0.0), 1.0e-12, "Internal bending forces must conserve translation.");
    checkVector(net_torque, glm::dvec3(0.0), 1.0e-13, "Internal bending forces must conserve angular momentum.");
    std::cout << "  maximum bending gradient finite-difference error: " << maximum_error << " N\n";
}

void testCurvedRestAndObjectivity()
{
    const FacialElasticSolver solver;
    const auto system = curvedStencil();
    check(glm::length(system.bending_stencils[0].rest_curvature_per_m) > 3.0,
          "Fixture must have actual nonzero rest curvature.");
    const auto neutral = solver.solve(system);
    check(neutral.converged && neutral.iterations == 0 && neutral.positions == system.rest_positions,
          "A curved neutral shell must remain exactly at its immutable rest state.");
    check(neutral.final_evaluation.bending_energy_j == 0.0 && neutral.free_force_l2_n == 0.0,
          "Rest curvature must be subtracted consistently, not flattened.");
    const auto positions = deformedStencil(system);
    const auto original = solver.evaluate(system, positions);
    const glm::dmat3 rotation(glm::rotate(glm::dmat4(1.0), 1.13,
                                         glm::normalize(glm::dvec3(-0.3, 0.7, 0.4))));
    const glm::dvec3 translation(0.31, -0.47, 0.19);
    auto moved = positions;
    for (auto &position : moved) position = rotation * position + translation;
    const auto transformed = solver.evaluate(system, moved);
    checkNear(transformed.bending_energy_j, original.bending_energy_j, 1.0e-16, 1.0e-10,
              "Bending energy must be invariant under rigid motion of the current shape.");
    checkNear(transformed.maximum_bending_curvature_change_per_m,
              original.maximum_bending_curvature_change_per_m, 1.0e-10, 1.0e-10,
              "Material curvature change must not depend on world frame.");
    for (std::size_t index = 0; index < moved.size(); ++index)
        checkVector(transformed.gradient_n[index], rotation * original.gradient_n[index], 2.0e-11,
                    "Bending gradient must rotate with the geometry.");
    auto moved_rest = system.rest_positions;
    for (auto &position : moved_rest) position = rotation * position + translation;
    check(solver.evaluate(system, moved_rest).bending_energy_j < 1.0e-24,
          "Rigid rotation of a curved rest shell must not cost bending energy.");
}

void testPhysicalScalingAndDisabledMode()
{
    const FacialElasticSolver solver;
    const auto system = curvedStencil();
    const auto positions = deformedStencil(system);
    const auto original = solver.evaluate(system, positions);
    auto scaled = system;
    scaled.bending_stencils[0].bending_rigidity_n_m *= 8.0;
    const auto thickened = solver.evaluate(scaled, positions);
    checkNear(thickened.bending_energy_j, 8.0 * original.bending_energy_j, 1.0e-16, 1.0e-13,
              "Doubling thickness must multiply pure bending rigidity by eight.");
    for (std::size_t index = 0; index < positions.size(); ++index)
        checkVector(thickened.gradient_n[index], 8.0 * original.gradient_n[index], 1.0e-12,
                    "Bending force must scale with rigidity.");
    checkNear(rigidity(40000.0, 0.002, 0.3), 8.0 * rigidity(40000.0, 0.001, 0.3), 0.0, 1.0e-14,
              "Physical D must use thickness cubed and the plane-stress denominator.");

    auto membrane = system;
    membrane.bending_stencils.clear();
    membrane.triangles.push_back(FacialElasticSolver::makeRestTriangle(
        {4, 5, 7}, membrane.rest_positions, 40000.0, 0.3, 0.001));
    auto disabled = membrane;
    disabled.bending_stencils = system.bending_stencils;
    disabled.bending_stencils[0].bending_rigidity_n_m = 0.0;
    const auto without_bending = solver.evaluate(membrane, positions);
    const auto zero_bending = solver.evaluate(disabled, positions);
    check(without_bending.total_energy_j == zero_bending.total_energy_j &&
          without_bending.gradient_n == zero_bending.gradient_n && zero_bending.bending_energy_j == 0.0,
          "Disabled bending must preserve the membrane-only objective exactly.");
    const auto without_solve = solver.solve(membrane);
    const auto zero_solve = solver.solve(disabled);
    check(without_solve.positions == zero_solve.positions &&
          without_solve.accepted_energies_j == zero_solve.accepted_energies_j &&
          without_solve.force_scale_n == zero_solve.force_scale_n,
          "Disabled bending must preserve the membrane-only solver path.");
}

void testCylinderOrientationAndRefinement()
{
    constexpr double young = 40000.0, thickness = 0.001, poisson = 0.35;
    constexpr double length = 0.1, curvature = 1.0;
    constexpr double pi = 3.1415926535897932384626433832795;
    const double expected = 0.5 * rigidity(young, thickness, poisson) * length * length * curvature * curvature;
    const FacialElasticSolver solver;
    for (bool checkerboard : {false, true}) {
        std::array<double, 3> previous_errors{{1.0, 1.0, 1.0}};
        for (int segments : {8, 16, 32}) {
            const FacialBendingSheetFixture sheet(segments, segments, length, length,
                                                  checkerboard, young, poisson, thickness);
            std::array<double, 3> ratios{};
            int orientation = 0;
            for (double angle : {0.0, pi / 4.0, -pi / 4.0}) {
                const auto evaluation = solver.evaluate(sheet.system, sheet.cylinder(curvature, angle));
                const double ratio = evaluation.bending_energy_j / expected;
                const double error = std::fabs(ratio - 1.0);
                check(error < 2.0e-4, "Cylinder bending must use the physical plate rigidity without a tuned factor.");
                check(error < 0.35 * previous_errors[orientation],
                      "Cylinder energy must converge under refinement for every mesh orientation.");
                previous_errors[orientation] = error;
                ratios[orientation++] = ratio;
            }
            std::cout << "  cylinder " << (checkerboard ? "checkerboard" : "uniform")
                      << " n=" << segments << " energy ratios at0,+45,-45deg: "
                      << ratios[0] << ", " << ratios[1] << ", " << ratios[2] << '\n';
        }
    }
}

double verifyCantileverResponse(int horizontal_segments, int vertical_segments, bool compare_stiffness)
{
    constexpr double length = 0.05, width = 0.01, thickness = 0.001, young = 1.0e6, poisson = 0.0;
    constexpr double total_force = 1.0e-6;
    FacialBendingSheetFixture sheet(horizontal_segments, vertical_segments, length, width,
                                    false, young, poisson, thickness, true);
    sheet.system.fixed_vertices.assign(sheet.system.rest_positions.size(), false);
    sheet.system.external_forces_n.assign(sheet.system.rest_positions.size(), glm::dvec3(0.0));
    for (int row = 0; row <= vertical_segments; ++row) {
        sheet.system.fixed_vertices[sheet.index(0, row)] = true;
        sheet.system.fixed_vertices[sheet.index(1, row)] = true;
        // Consistent endpoint trapezoidal weights give a uniform line load.
        const double weight = (row == 0 || row == vertical_segments) ? 0.5 : 1.0;
        sheet.system.external_forces_n[sheet.index(horizontal_segments, row)].z =
            total_force * weight / vertical_segments;
    }
    FacialElasticSolverSettings settings;
    settings.max_iterations = 2000;
    settings.relative_force_tolerance = 1.0e-7;
    const FacialElasticSolver solver;
    const auto result = solver.solve(sheet.system, settings);
    std::cout << "  cantilever " << horizontal_segments << 'x' << vertical_segments
              << " termination: " << result.termination_reason << ", iterations="
              << result.iterations << ", relative residual=" << result.relative_free_force << '\n';
    check(result.converged && result.relative_free_force <= settings.relative_force_tolerance,
          "Loaded cantilever must achieve actual force equilibrium.");
    check(result.free_force_l2_n < 1.0e-3 * total_force,
          "Cantilever force imbalance must also be small relative to its applied load.");
    double deflection = 0.0;
    for (int row = 0; row <= vertical_segments; ++row) {
        const double weight = (row == 0 || row == vertical_segments) ? 0.5 : 1.0;
        deflection += weight * result.positions[sheet.index(horizontal_segments, row)].z / vertical_segments;
    }
    // Two fixed grid rows impose a discrete zero slope about their midpoint.
    // Compare against a beam with that explicitly measured clamp location;
    // this distance tends to the geometric boundary under mesh refinement.
    const double free_length = length - 0.5 * length / horizontal_segments;
    const double expected = total_force * free_length * free_length * free_length /
                            (3.0 * rigidity(young, thickness, poisson) * width);
    checkNear(deflection, expected, 0.0, 0.12,
              "Small-load cantilever displacement must agree with the Euler-Bernoulli scale.");
    check(result.final_evaluation.bending_energy_j > 0.0,
          "A loaded cantilever must store nonzero bending energy.");
    checkMonotone(result);
    std::cout << "  cantilever tip=" << deflection << " m; beam oracle=" << expected << " m\n";
    const double relative_error = std::fabs(deflection / expected - 1.0);
    if (!compare_stiffness) return relative_error;
    auto doubled = sheet.system;
    for (auto &stencil : doubled.bending_stencils) stencil.bending_rigidity_n_m *= 2.0;
    const auto stiffer = solver.solve(doubled, settings);
    check(stiffer.converged, "Stiffer cantilever must achieve force equilibrium.");
    double stiffer_deflection = 0.0;
    for (int row = 0; row <= vertical_segments; ++row) {
        const double weight = (row == 0 || row == vertical_segments) ? 0.5 : 1.0;
        stiffer_deflection += weight * stiffer.positions[sheet.index(horizontal_segments, row)].z / vertical_segments;
    }
    checkNear(stiffer_deflection / deflection, 0.5, 0.0, 0.01,
              "Doubling bending rigidity must halve small-load cantilever deflection.");
    checkMonotone(stiffer);
    return relative_error;
}

void testCantileverResponse()
{
    const double coarse_error = verifyCantileverResponse(8, 2, true);
    const double refined_error = verifyCantileverResponse(12, 3, false);
    check(refined_error < 0.7 * coarse_error,
          "Cantilever displacement error must decrease under mesh refinement.");
}

void testDegenerateNormalRejection()
{
    const FacialElasticSolver solver;
    auto system = curvedStencil();
    auto collapsed = system.rest_positions;
    collapsed[5] = collapsed[4];
    bool rejected = false;
    try { solver.evaluate(system, collapsed); } catch (const std::overflow_error &) { rejected = true; }
    check(rejected, "A singular current normal must not become zero bending energy.");
    system.bending_stencils[0].bending_rigidity_n_m = -1.0;
    rejected = false;
    try { solver.solve(system); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Negative bending rigidity must be rejected.");
}

} // namespace

int main()
{
    std::cout << std::setprecision(12);
    const std::pair<const char *, std::function<void()>> tests[] = {
        {"exact bending energy gradient", testExactBendingGradient},
        {"curved rest state and objectivity", testCurvedRestAndObjectivity},
        {"physical scaling and disabled compatibility", testPhysicalScalingAndDisabledMode},
        {"cylinder orientation and refinement", testCylinderOrientationAndRefinement},
        {"cantilever force equilibrium and stiffness", testCantileverResponse},
        {"singular geometry and invalid rigidity", testDegenerateNormalRejection}};
    std::size_t passed = 0;
    for (const auto &test : tests) {
        try {
            test.second();
            ++passed;
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception &error) {
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
            return 1;
        }
    }
    std::cout << "PASS facial bending kernel: " << passed << " numerical test groups\n";
    return 0;
}
