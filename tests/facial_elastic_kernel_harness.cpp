#include "geometry/service/FacialElasticSolver.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void check(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkNear(double actual, double expected, double absolute_tolerance,
               double relative_tolerance, const std::string &message)
{
    const double error = std::fabs(actual - expected);
    const double tolerance = absolute_tolerance + relative_tolerance *
        std::max(std::fabs(actual), std::fabs(expected));
    if (!std::isfinite(actual) || error > tolerance)
        throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                                 ", expected=" + std::to_string(expected));
}

void checkVector(const glm::dvec3 &actual, const glm::dvec3 &expected,
                 double tolerance, const std::string &message)
{
    check(std::isfinite(glm::length(actual)) && glm::length(actual - expected) <= tolerance,
          message);
}

void checkMonotone(const FacialElasticSolveResult &result)
{
    check(result.accepted_energies_j.size() == result.iterations + 1,
          "Energy history must contain the initial state and every accepted state.");
    for (std::size_t index = 1; index < result.accepted_energies_j.size(); ++index)
        check(result.accepted_energies_j[index] <= result.accepted_energies_j[index - 1],
              "Accepted energy must not increase.");
}

FacialElasticSolverSettings accurateSettings()
{
    FacialElasticSolverSettings settings;
    settings.max_iterations = 2000;
    settings.relative_force_tolerance = 1.0e-8;
    return settings;
}

FacialElasticSystem pointSystem(double support_stiffness = 80.0)
{
    FacialElasticSystem system;
    system.rest_positions = {{0.0, 0.0, 0.0}};
    system.support_targets = system.rest_positions;
    system.support_stiffness_n_per_m = {support_stiffness};
    FacialElasticMuscle muscle;
    muscle.name = "point-cable";
    muscle.insertions = {{0, 1.0}};
    muscle.origin = {0.02, 0.0, 0.0};
    muscle.rest_length_m = 0.02;
    muscle.activation = 0.5;
    muscle.maximum_shortening = 0.2;
    muscle.stiffness_n_per_m = 20.0;
    system.muscles.push_back(muscle);
    return system;
}

FacialElasticSystem coupledPatch()
{
    FacialElasticSystem system;
    system.rest_positions = {{0.0, 0.0, 0.0}, {0.02, 0.0, 0.004},
                             {0.0, 0.015, 0.001}, {0.02, 0.015, 0.005}};
    system.triangles = {
        FacialElasticSolver::makeRestTriangle({0, 1, 3}, system.rest_positions, 12000.0, 0.29, 0.0012),
        FacialElasticSolver::makeRestTriangle({0, 3, 2}, system.rest_positions, 18000.0, 0.22, 0.0012)};
    system.support_targets = system.rest_positions;
    system.support_stiffness_n_per_m = {2.0, 3.0, 5.0, 7.0};
    system.external_forces_n = {{0.0003, 0.0002, -0.0001}, {0.0, 0.0, 0.0},
                               {-0.0001, 0.0004, 0.0002}, {0.0, -0.0002, 0.0001}};
    FacialElasticMuscle muscle;
    muscle.name = "weighted-cable";
    muscle.insertions = {{2, 0.3}, {3, 0.7}};
    muscle.origin = {0.03, 0.03, 0.025};
    muscle.rest_length_m = glm::length(0.3 * system.rest_positions[2] +
                                      0.7 * system.rest_positions[3] - muscle.origin);
    muscle.activation = 0.4;
    muscle.maximum_shortening = 0.22;
    muscle.stiffness_n_per_m = 18.0;
    system.muscles.push_back(muscle);
    return system;
}

std::vector<glm::dvec3> deformedPatch(const FacialElasticSystem &system)
{
    auto positions = system.rest_positions;
    positions[0] += glm::dvec3(-0.0002, 0.0001, 0.0003);
    positions[1] += glm::dvec3(0.0011, 0.0004, -0.0002);
    positions[2] += glm::dvec3(-0.0006, -0.0005, 0.0007);
    positions[3] += glm::dvec3(0.0005, 0.0008, 0.0004);
    return positions;
}

void testEnergyGradient()
{
    const FacialElasticSolver solver;
    const auto system = coupledPatch();
    const auto positions = deformedPatch(system);
    const auto evaluation = solver.evaluate(system, positions);
    const double step = 1.0e-8;
    double largest_error = 0.0;
    for (std::size_t index = 0; index < positions.size(); ++index) {
        for (int axis = 0; axis < 3; ++axis) {
            auto positive = positions;
            auto negative = positions;
            positive[index][axis] += step;
            negative[index][axis] -= step;
            const double derivative = (solver.evaluate(system, positive).total_energy_j -
                                       solver.evaluate(system, negative).total_energy_j) / (2.0 * step);
            largest_error = std::max(largest_error, std::fabs(derivative - evaluation.gradient_n[index][axis]));
            checkNear(evaluation.gradient_n[index][axis], derivative, 2.0e-10, 2.0e-6,
                      "Exact energy gradient failed its central finite difference.");
        }
    }
    checkNear(evaluation.total_energy_j, evaluation.membrane_energy_j + evaluation.support_energy_j +
              evaluation.muscle_energy_j + evaluation.external_potential_j, 0.0, 0.0,
              "Reported energy components do not sum to the objective.");
    check(evaluation.maximum_green_strain > 0.0 && evaluation.maximum_green_strain < 0.2,
          "Deformed patch must report a nonzero finite principal Green strain.");
    check(evaluation.muscle_states.size() == 1 && evaluation.muscle_states[0].name == "weighted-cable",
          "Named muscle diagnostics are missing.");
    const auto &muscle = system.muscles.front();
    const auto &state = evaluation.muscle_states.front();
    checkVector(state.insertion_position, 0.3 * positions[2] + 0.7 * positions[3], 1.0e-16,
                "Muscle insertion must use normalized patch weights.");
    checkNear(state.tension_n, muscle.stiffness_n_per_m * state.extension_m, 1.0e-16, 1.0e-14,
              "Muscle tension diagnostic differs from the energy derivative.");
    std::cout << "  maximum gradient finite-difference error: " << largest_error << " N\n";
}

void testPointOracleAndStiffness()
{
    const FacialElasticSolver solver;
    const auto settings = accurateSettings();
    const auto system = pointSystem();
    const auto result = solver.solve(system, settings);
    check(result.converged && result.admissible, "One-point spring/cable solve did not converge.");
    checkVector(result.positions[0], {0.0004, 0.0, 0.0}, 1.0e-12,
                "One-point spring/cable displacement must be exactly 0.4 mm.");
    checkNear(result.final_evaluation.muscle_tensions_n[0], 0.032, 1.0e-12, 1.0e-10,
              "One-point equilibrium cable tension must be 0.032 N.");
    checkVector(result.final_evaluation.support_forces_n[0], {-0.032, 0.0, 0.0}, 1.0e-11,
                "Support force must balance cable tension.");
    checkMonotone(result);

    const auto stiffer = solver.solve(pointSystem(320.0), settings);
    const double expected_stiffer = 20.0 / 340.0 * 0.5 * 0.2 * 0.02;
    check(stiffer.converged, "Stiffer one-point system did not converge.");
    checkNear(stiffer.positions[0].x, expected_stiffer, 1.0e-12, 1.0e-10,
              "Support stiffness must change equilibrium by the analytic force balance.");
    check(stiffer.positions[0].x < result.positions[0].x,
          "Increasing tissue support stiffness must reduce this cable-driven displacement.");

    auto scaled = system;
    scaled.support_stiffness_n_per_m[0] *= 10.0;
    scaled.muscles[0].stiffness_n_per_m *= 10.0;
    const auto scaled_result = solver.solve(scaled, settings);
    check(scaled_result.converged, "Uniformly stiffness-scaled point system did not converge.");
    checkVector(scaled_result.positions[0], result.positions[0], 1.0e-12,
                "Uniform stiffness scaling must preserve force-balanced geometry.");
    checkNear(scaled_result.final_evaluation.total_energy_j, 10.0 * result.final_evaluation.total_energy_j,
              1.0e-15, 1.0e-10, "Uniform stiffness scaling must scale energy.");
    checkNear(scaled_result.force_scale_n, 10.0 * result.force_scale_n, 1.0e-14, 1.0e-10,
              "Residual normalization must scale with stiffness.");
    checkMonotone(scaled_result);
}

void testNeutralAndTensileOnly()
{
    const FacialElasticSolver solver;
    for (int disabled_term = 0; disabled_term < 3; ++disabled_term) {
        auto system = pointSystem();
        if (disabled_term == 0) system.muscles[0].activation = 0.0;
        if (disabled_term == 1) system.muscles[0].maximum_shortening = 0.0;
        if (disabled_term == 2) system.muscles[0].stiffness_n_per_m = 0.0;
        const auto result = solver.solve(system);
        check(result.converged && result.iterations == 0, "Zero-load point system must be immediately neutral.");
        checkVector(result.positions[0], system.rest_positions[0], 0.0,
                    "Zero activation, shortening, or cable stiffness must not create motion.");
        check(result.final_evaluation.muscle_tensions_n[0] == 0.0,
              "Disabled muscle must not retain tension.");
    }
    auto slack = pointSystem();
    slack.muscles[0].activation = 0.0;
    slack.muscles[0].rest_length_m = 0.03;
    const auto slack_evaluation = solver.evaluate(slack, slack.rest_positions);
    check(slack_evaluation.muscle_energy_j == 0.0 && slack_evaluation.muscle_tensions_n[0] == 0.0,
          "Slack muscle cable must not push the skin.");

    auto neutral = coupledPatch();
    neutral.external_forces_n.clear();
    neutral.muscles[0].activation = 0.0;
    const auto result = solver.solve(neutral);
    check(result.converged && result.iterations == 0,
          "Neutral curved-in-world membrane must tolerate floating-point roundoff without moving.");
    check(result.positions == neutral.rest_positions, "Neutral membrane must preserve immutable rest coordinates.");
    check(result.relative_free_force <= 1.0e-4, "Neutral membrane residual has an invalid normalization.");
    check(result.final_evaluation.membrane_energy_j < 1.0e-29,
          "Neutral rest triangle frame must have negligible membrane energy.");
}

void testRigidFrameObjectivity()
{
    const FacialElasticSolver solver;
    const auto original = coupledPatch();
    const auto original_positions = deformedPatch(original);
    const auto original_evaluation = solver.evaluate(original, original_positions);
    const glm::dmat3 rotation(glm::rotate(glm::dmat4(1.0), 0.73,
                                         glm::normalize(glm::dvec3(0.3, -0.6, 0.7))));
    const glm::dvec3 translation(0.31, -0.47, 0.19);
    auto transformed = original;
    auto transformed_positions = original_positions;
    for (std::size_t index = 0; index < original_positions.size(); ++index) {
        transformed.rest_positions[index] = rotation * original.rest_positions[index] + translation;
        transformed.support_targets[index] = rotation * original.support_targets[index] + translation;
        transformed.external_forces_n[index] = rotation * original.external_forces_n[index];
        transformed_positions[index] = rotation * original_positions[index] + translation;
    }
    transformed.muscles[0].origin = rotation * original.muscles[0].origin + translation;
    const auto transformed_evaluation = solver.evaluate(transformed, transformed_positions);
    checkNear(transformed_evaluation.total_energy_j, original_evaluation.total_energy_j,
              1.0e-16, 1.0e-11, "Rigid change of world frame must preserve the total potential.");
    checkNear(transformed_evaluation.maximum_green_strain, original_evaluation.maximum_green_strain,
              1.0e-13, 1.0e-11, "Principal Green strain must be frame invariant.");
    for (std::size_t index = 0; index < original_positions.size(); ++index)
        checkVector(transformed_evaluation.gradient_n[index], rotation * original_evaluation.gradient_n[index],
                    2.0e-13, "Gradient must transform as a world-space vector.");

    auto membrane_only = original;
    membrane_only.support_targets.clear();
    membrane_only.support_stiffness_n_per_m.clear();
    membrane_only.external_forces_n.clear();
    membrane_only.muscles.clear();
    auto moved_rest = original.rest_positions;
    for (auto &position : moved_rest) position = rotation * position + translation;
    check(solver.evaluate(membrane_only, moved_rest).membrane_energy_j < 1.0e-27,
          "A freely rotated and translated membrane must have zero strain energy.");
}

FacialElasticSystem stripSystem(double force = 0.002)
{
    constexpr double length = 0.1;
    constexpr double width = 0.02;
    FacialElasticSystem strip;
    strip.rest_positions = {{0.0, 0.0, 0.0}, {length, 0.0, 0.0},
                            {length, width, 0.0}, {0.0, width, 0.0}};
    strip.triangles = {
        FacialElasticSolver::makeRestTriangle({0, 1, 2}, strip.rest_positions, 1.0e5, 0.3, 0.001),
        FacialElasticSolver::makeRestTriangle({0, 2, 3}, strip.rest_positions, 1.0e5, 0.3, 0.001)};
    strip.fixed_vertices = {true, false, false, false};
    strip.external_forces_n = {{-force / 2.0, 0.0, 0.0}, {force / 2.0, 0.0, 0.0},
                              {force / 2.0, 0.0, 0.0}, {-force / 2.0, 0.0, 0.0}};
    return strip;
}

void testMembraneStripOracle()
{
    const FacialElasticSolver solver;
    const auto strip = stripSystem();
    auto settings = accurateSettings();
    // Three orders tighter than the production residual gate; pushing below
    // this scale makes strict Armijo decreases sensitive to energy roundoff.
    settings.relative_force_tolerance = 1.0e-7;
    const auto result = solver.solve(strip, settings);
    std::cout << "  strip solve: " << result.termination_reason << ", iterations: "
              << result.iterations << ", relative residual: " << result.relative_free_force << '\n';
    check(result.converged && result.relative_free_force <= settings.relative_force_tolerance,
          "Membrane strip must satisfy the measured free-force residual.");
    // For free lateral traction, Syy=0 gives eyy=-nu*exx. The nominal axial
    // traction is Pxx=E*lambda_x*(lambda_x^2-1)/2 for this plane-stress model.
    const double nominal_strain = 0.002 / (1.0e5 * 0.001 * 0.02);
    double stretch_x = 1.0 + nominal_strain;
    for (int iteration = 0; iteration < 12; ++iteration)
        stretch_x -= (0.5 * stretch_x * (stretch_x * stretch_x - 1.0) - nominal_strain) /
                     (1.5 * stretch_x * stretch_x - 0.5);
    const double stretch_y = std::sqrt(1.0 - 0.3 * (stretch_x * stretch_x - 1.0));
    for (std::size_t index = 0; index < strip.rest_positions.size(); ++index) {
        const glm::dvec3 expected(strip.rest_positions[index].x * stretch_x,
                                  strip.rest_positions[index].y * stretch_y, 0.0);
        checkVector(result.positions[index], expected, 2.0e-9,
                    "Finite-element strip differs from homogeneous plane-stress equilibrium.");
    }
    const double axial_extension = 0.5 * (result.positions[1].x + result.positions[2].x -
                                        result.positions[0].x - result.positions[3].x) - 0.1;
    const double lateral_extension = 0.5 * (result.positions[2].y + result.positions[3].y -
                                          result.positions[0].y - result.positions[1].y) - 0.02;
    checkNear(axial_extension, 0.1 * nominal_strain, 0.0, 0.002,
              "Small-load axial extension must approach FL/(E*h*width).");
    checkNear(-lateral_extension / 0.02 / (axial_extension / 0.1), 0.3, 0.0, 0.002,
              "Small-load lateral contraction must approach the specified Poisson ratio.");
    checkVector(result.positions[0], strip.rest_positions[0], 0.0,
                "A fixed membrane vertex must remain bit-exact at rest.");
    checkMonotone(result);
    std::cout << "  strip axial extension: " << axial_extension * 1.0e3
              << " mm, lateral extension: " << lateral_extension * 1.0e3
              << " mm, iterations: " << result.iterations
              << ", relative residual: " << result.relative_free_force << '\n';

    auto stiffer = strip;
    for (auto &triangle : stiffer.triangles) triangle.young_modulus_pa *= 2.0;
    const auto stiffer_result = solver.solve(stiffer, settings);
    check(stiffer_result.converged, "Stiffer membrane strip must converge.");
    const double stiffer_extension = 0.5 * (stiffer_result.positions[1].x + stiffer_result.positions[2].x -
                                          stiffer_result.positions[0].x - stiffer_result.positions[3].x) - 0.1;
    checkNear(stiffer_extension / axial_extension, 0.5, 0.0, 0.002,
              "Doubling Young's modulus must halve small-strain extension.");

    auto scaled = strip;
    for (auto &triangle : scaled.triangles) triangle.young_modulus_pa *= 10.0;
    for (auto &force : scaled.external_forces_n) force *= 10.0;
    const auto scaled_result = solver.solve(scaled, settings);
    check(scaled_result.converged, "Force and stiffness scaled membrane strip must converge.");
    for (std::size_t index = 0; index < result.positions.size(); ++index)
        checkVector(scaled_result.positions[index], result.positions[index], 3.0e-9,
                    "Scaling all stiffnesses and loads must preserve the strip equilibrium.");
    checkNear(scaled_result.force_scale_n, 10.0 * result.force_scale_n, 1.0e-13, 1.0e-10,
              "Membrane residual normalization must be homogeneous in forces and stiffness.");
}

void testNonconvergenceAndAdmissibility()
{
    const FacialElasticSolver solver;
    auto settings = accurateSettings();
    settings.max_iterations = 1;
    const auto limited = solver.solve(stripSystem(), settings);
    check(!limited.converged && limited.relative_free_force > settings.relative_force_tolerance,
          "Iteration exhaustion must never be reported as force equilibrium.");
    check(limited.termination_reason == "iteration budget exhausted" && limited.iterations == 1,
          "Iteration budget failure must identify the actual stopping reason.");
    checkMonotone(limited);

    const auto point = pointSystem();
    const auto blocked = solver.solve(point, accurateSettings(),
        [&point](const std::vector<glm::dvec3> &positions) { return positions == point.rest_positions; });
    check(!blocked.converged && blocked.admissible && blocked.iterations == 0,
          "Geometry-blocked loads must retain an admissible but unconverged state.");
    check(blocked.positions == point.rest_positions && blocked.free_force_l2_n > 0.03,
          "Admissibility must not hide a clamp force or erase the force residual.");
    check(blocked.termination_reason == "line search blocked by geometry admissibility",
          "Geometry rejection must be distinguishable from numerical failure.");
    checkMonotone(blocked);

    const auto invalid_initial = solver.solve(point, accurateSettings(),
        [](const std::vector<glm::dvec3> &) { return false; });
    check(!invalid_initial.admissible && !invalid_initial.converged,
          "Initial infeasibility must be explicit.");

    auto fixed = point;
    fixed.fixed_vertices = {true};
    const auto reaction = solver.solve(fixed);
    check(reaction.converged && reaction.free_force_l2_n == 0.0 && reaction.positions == fixed.rest_positions,
          "Fixed vertices must not enter the free-force residual or move.");
    checkNear(reaction.final_evaluation.gradient_n[0].x, -0.04, 1.0e-14, 1.0e-12,
              "Full objective gradient at a fixed vertex must retain its reaction information.");
}

void testReflectionAndInputRejection()
{
    const FacialElasticSolver solver;
    auto membrane = stripSystem();
    membrane.external_forces_n.clear();
    auto reflected = membrane.rest_positions;
    for (auto &position : reflected) position.x = -position.x;
    check(solver.evaluate(membrane, reflected).membrane_energy_j < 1.0e-28,
          "Test fixture must expose StVK's reflection blindness.");
    const auto &triangle = membrane.triangles.front().vertex_indices;
    const double projected_area = glm::cross(reflected[triangle[1]] - reflected[triangle[0]],
                                             reflected[triangle[2]] - reflected[triangle[0]]).z;
    check(projected_area < 0.0, "A separate orientation predicate must detect the reflected membrane.");

    auto bad_weights = pointSystem();
    bad_weights.muscles[0].insertions[0].weight = 0.7;
    bool rejected = false;
    try { solver.solve(bad_weights); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Unnormalized muscle weights must be rejected, not silently alter effective stiffness.");
    auto invalid_material = stripSystem();
    invalid_material.triangles[0].poisson_ratio = 0.5;
    rejected = false;
    try { solver.solve(invalid_material); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Unsupported material parameters must be rejected explicitly.");
    auto nonfinite = pointSystem();
    nonfinite.muscles[0].origin.x = std::numeric_limits<double>::quiet_NaN();
    rejected = false;
    try { solver.solve(nonfinite); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "Nonfinite model input must be rejected before minimization.");
}

} // namespace

int main()
{
    std::cout << std::setprecision(12);
    const std::pair<const char *, std::function<void()>> tests[] = {
        {"exact energy gradient", testEnergyGradient},
        {"point oracle and stiffness response", testPointOracleAndStiffness},
        {"neutral state and tensile-only cable", testNeutralAndTensileOnly},
        {"rigid-frame objectivity", testRigidFrameObjectivity},
        {"plane-stress strip oracle", testMembraneStripOracle},
        {"honest nonconvergence and fixed reactions", testNonconvergenceAndAdmissibility},
        {"orientation separation and input validation", testReflectionAndInputRejection}};
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
    std::cout << "PASS facial elastic kernel: " << passed << " numerical test groups\n";
    return 0;
}
