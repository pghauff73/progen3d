#include "geometry/service/FacialBendingStencilAssembler.h"
#include "geometry/service/FacialElasticSolver.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
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
    check(std::isfinite(actual) && std::fabs(actual - expected) <= absolute_tolerance +
          relative_tolerance * std::max(std::fabs(actual), std::fabs(expected)), message);
}

FacialElasticSystem boundaryPatch()
{
    FacialElasticSystem system;
    // The first triangle's one-ring contains only vertices0..4, so no six-term
    // quadratic reconstruction exists. Its second ring introduces vertices5..7
    // and has full rank. Fixing only0..4 must not discard this real coupling.
    system.rest_positions = {{0.0, 0.0, 0.0}, {0.01, 0.0, 0.0}, {0.0, 0.01, 0.0},
                             {0.01, 0.01, 0.0}, {0.02, 0.0, 0.0}, {0.02, 0.01, 0.0},
                             {0.02, 0.02, 0.0}, {0.01, 0.02, 0.0}};
    const std::array<std::size_t, 3> faces[] = {
        {0, 1, 2}, {1, 3, 2}, {1, 4, 3}, {4, 5, 3}, {3, 5, 6}, {3, 6, 7}};
    for (const auto &face : faces)
        system.triangles.push_back(FacialElasticSolver::makeRestTriangle(
            face, system.rest_positions, 40000.0, 0.3, 0.001));
    return system;
}

FacialElasticSystem bendingOnly(FacialElasticSystem system)
{
    system.triangles.clear();
    return system;
}

void testGeometrySelectedFixedBoundary()
{
    auto unconstrained = boundaryPatch();
    auto constrained = unconstrained;
    constrained.fixed_vertices = {true, true, true, true, true, false, false, false};
    const FacialBendingStencilAssembler assembler;
    const auto all_diagnostics = assembler.assemble(unconstrained, 1.0);
    const auto fixed_diagnostics = assembler.assemble(constrained, 1.0);
    check(all_diagnostics.expanded_stencils > 0 && fixed_diagnostics.expanded_stencils > 0,
          "The fixture must require a larger geometry-selected reconstruction patch.");
    check(fixed_diagnostics.active_stencils == all_diagnostics.active_stencils &&
          constrained.bending_stencils.size() == unconstrained.bending_stencils.size(),
          "Fixing the deficient first ring must not remove a stencil coupled to a free second ring.");
    check(fixed_diagnostics.fixed_only_triangles == 0,
          "Every accepted fixture patch contains a free vertex.");
    check(constrained.bending_stencils.front().normal_vertex_indices ==
          std::array<std::size_t, 3>{{0, 1, 2}}, "The fixed central triangle must retain bending coupling.");
    bool nonzero_free_weight = false;
    for (const auto &weight : constrained.bending_stencils.front().curvature_weights)
        if (!constrained.fixed_vertices[weight.vertex_index] && glm::length(weight.weights_per_m2) > 0.0)
            nonzero_free_weight = true;
    check(nonzero_free_weight, "The central stencil must actually depend on its free outer neighbors.");
    for (std::size_t index = 0; index < unconstrained.bending_stencils.size(); ++index) {
        const auto &first = unconstrained.bending_stencils[index];
        const auto &second = constrained.bending_stencils[index];
        check(first.normal_vertex_indices == second.normal_vertex_indices &&
              first.curvature_weights.size() == second.curvature_weights.size(),
              "A fixed mask must not change reconstruction neighborhoods.");
        for (std::size_t weight_index = 0; weight_index < first.curvature_weights.size(); ++weight_index) {
            check(first.curvature_weights[weight_index].vertex_index == second.curvature_weights[weight_index].vertex_index &&
                  first.curvature_weights[weight_index].weights_per_m2 == second.curvature_weights[weight_index].weights_per_m2,
                  "A fixed mask must not change an active curvature operator.");
        }
    }
    auto positions = unconstrained.rest_positions;
    positions[5].z = 0.00012;
    positions[6].z = -0.00007;
    positions[7].z = 0.00005;
    const FacialElasticSolver solver;
    const auto first = solver.evaluate(bendingOnly(unconstrained), positions);
    const auto second = solver.evaluate(bendingOnly(constrained), positions);
    check(first.bending_energy_j > 0.0 && first.bending_energy_j == second.bending_energy_j &&
          first.gradient_n == second.gradient_n,
          "Constraint elimination must preserve the physical bending objective and full reactions.");
    const double physical_rigidity = 40000.0 * std::pow(0.001, 3) / (12.0 * (1.0 - 0.3 * 0.3));
    checkNear(constrained.bending_stencils.front().bending_rigidity_n_m, physical_rigidity,
              1.0e-20, 1.0e-14, "Assembler must use the declared physical plate rigidity.");
    std::cout << "  active=" << fixed_diagnostics.active_stencils
              << " expanded=" << fixed_diagnostics.expanded_stencils
              << " maximum rings=" << fixed_diagnostics.maximum_neighborhood_rings
              << " reproduction error=" << fixed_diagnostics.maximum_polynomial_reproduction_error << '\n';
}

void testChartAndWorldFrameInvariance()
{
    const FacialBendingStencilAssembler assembler;
    const FacialElasticSolver solver;
    auto original = boundaryPatch();
    const auto diagnostic = assembler.assemble(original, 1.0);
    const auto neutral = solver.solve(bendingOnly(original));
    check(neutral.converged && neutral.iterations == 0 && neutral.final_evaluation.bending_energy_j == 0.0,
          "A production-assembled neutral patch must have exactly zero bending energy.");
    auto positions = original.rest_positions;
    positions[2] += glm::dvec3(-0.00003, 0.00002, 0.00004);
    positions[5] += glm::dvec3(0.00002, -0.00004, 0.00013);
    positions[6] += glm::dvec3(0.0, 0.00003, -0.00006);
    positions[7] += glm::dvec3(-0.00001, 0.00001, 0.00007);
    const auto reference = solver.evaluate(bendingOnly(original), positions);

    // Cyclic vertex order preserves the oriented triangle but changes the
    // material tangent basis selected by the assembler's first edge.
    auto reordered = original;
    reordered.triangles.clear();
    for (const auto &triangle : original.triangles) {
        const auto &indices = triangle.vertex_indices;
        reordered.triangles.push_back(FacialElasticSolver::makeRestTriangle(
            {indices[1], indices[2], indices[0]}, reordered.rest_positions,
            triangle.young_modulus_pa, triangle.poisson_ratio, triangle.thickness_m));
    }
    assembler.assemble(reordered, 1.0);
    const auto changed_chart = solver.evaluate(bendingOnly(reordered), positions);
    checkNear(changed_chart.bending_energy_j, reference.bending_energy_j, 1.0e-20, 1.0e-10,
              "Changing the tangent coordinate basis must preserve isotropic curvature energy.");
    for (std::size_t index = 0; index < positions.size(); ++index)
        check(glm::length(changed_chart.gradient_n[index] - reference.gradient_n[index]) < 1.0e-13,
              "Changing the tangent coordinate basis must preserve the physical force.");

    const glm::dmat3 rotation(glm::rotate(glm::dmat4(1.0), 1.02,
                                         glm::normalize(glm::dvec3(0.6, -0.2, 0.5))));
    const glm::dvec3 translation(0.7, -0.3, 0.2);
    auto moved = original;
    auto moved_positions = positions;
    for (std::size_t index = 0; index < positions.size(); ++index) {
        moved.rest_positions[index] = rotation * original.rest_positions[index] + translation;
        moved_positions[index] = rotation * positions[index] + translation;
    }
    const auto moved_diagnostic = assembler.assemble(moved, 1.0);
    const auto changed_frame = solver.evaluate(bendingOnly(moved), moved_positions);
    checkNear(changed_frame.bending_energy_j, reference.bending_energy_j, 1.0e-20, 1.0e-10,
              "Reassembling in a rigidly changed world frame must preserve energy.");
    checkNear(moved_diagnostic.maximum_reconstruction_condition,
              diagnostic.maximum_reconstruction_condition, 1.0e-9, 1.0e-10,
              "A rigid change of world frame must preserve reconstruction conditioning.");
    for (std::size_t index = 0; index < positions.size(); ++index)
        check(glm::length(changed_frame.gradient_n[index] - rotation * reference.gradient_n[index]) < 1.0e-13,
              "Reassembled bending forces must rotate with the world frame.");
}

void testFixedAndUnreconstructableAccounting()
{
    const FacialBendingStencilAssembler assembler;
    auto all_fixed = boundaryPatch();
    all_fixed.fixed_vertices.assign(all_fixed.rest_positions.size(), true);
    const auto valid_omission = assembler.assemble(all_fixed, 1.0);
    check(all_fixed.bending_stencils.empty() && valid_omission.fixed_only_triangles == all_fixed.triangles.size() &&
          valid_omission.fixed_only_unreconstructable_triangles == 0,
          "Valid fully fixed stencils must be counted separately from failed reconstruction.");

    FacialElasticSystem triangle;
    triangle.rest_positions = {{0.0, 0.0, 0.0}, {0.01, 0.0, 0.0}, {0.0, 0.01, 0.0}};
    triangle.triangles.push_back(FacialElasticSolver::makeRestTriangle(
        {0, 1, 2}, triangle.rest_positions, 40000.0, 0.3, 0.001));
    triangle.fixed_vertices = {true, true, true};
    const auto invalid_omission = assembler.assemble(triangle, 1.0);
    check(invalid_omission.fixed_only_triangles == 1 && invalid_omission.fixed_only_unreconstructable_triangles == 1 &&
          invalid_omission.active_stencils == 0,
          "An unreconstructable fully fixed component must have an explicit omission count.");
    triangle.fixed_vertices[2] = false;
    bool rejected = false;
    try { assembler.assemble(triangle, 1.0); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected, "An unreconstructable component with a free vertex must fail, not lose its bending term.");
    const auto disabled = assembler.assemble(triangle, 0.0);
    check(triangle.bending_stencils.empty() && disabled.active_stencils == 0 && disabled.fixed_only_triangles == 0,
          "Disabled bending must bypass curvature reconstruction without changing the membrane model.");
}

} // namespace

int main()
{
    try {
        std::cout << std::setprecision(12);
        testGeometrySelectedFixedBoundary();
        std::cout << "PASS geometry-selected fixed/free boundary coupling\n";
        testChartAndWorldFrameInvariance();
        std::cout << "PASS production reconstruction chart and world-frame invariance\n";
        testFixedAndUnreconstructableAccounting();
        std::cout << "PASS fixed-only and unreconstructable stencil accounting\n";
        std::cout << "PASS facial bending assembly: 3 numerical test groups\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL facial bending assembly: " << error.what() << '\n';
        return 1;
    }
}
