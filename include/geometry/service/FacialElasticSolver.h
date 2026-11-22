#pragma once

#include "geometry/model/FacialElasticSystem.h"

#include <functional>

// Static energy minimization. There is no timestep, inertia, damping or hidden
// target deformation. StVK is reflection-blind; the caller must supply any
// orientation/contact admissibility rules required by its geometry.
class FacialElasticSolver
{
public:
    using AdmissibilityCallback =
        std::function<bool(const std::vector<glm::dvec3> &)>;

    static FacialElasticTriangle makeRestTriangle(
        std::array<std::size_t, 3> vertex_indices,
        const std::vector<glm::dvec3> &rest_positions,
        double young_modulus_pa,
        double poisson_ratio,
        double thickness_m);

    // The assembler supplies affine-annihilating quadratic reconstruction
    // weights. Curvature is evaluated relative to the first normal vertex to
    // avoid cancellation under translation; this factory uses the same rule
    // for immutable rest curvature as the energy evaluator.
    static FacialElasticBendingStencil makeRestBendingStencil(
        std::array<std::size_t, 3> normal_vertex_indices,
        std::vector<FacialElasticCurvatureWeight> curvature_weights,
        const std::vector<glm::dvec3> &rest_positions,
        double bending_rigidity_n_m,
        double poisson_ratio);

    FacialElasticEnergyEvaluation evaluate(
        const FacialElasticSystem &system,
        const std::vector<glm::dvec3> &positions) const;

    FacialElasticSolveResult solve(
        const FacialElasticSystem &system,
        const FacialElasticSolverSettings &settings = {},
        AdmissibilityCallback admissibility = {}) const;
};
