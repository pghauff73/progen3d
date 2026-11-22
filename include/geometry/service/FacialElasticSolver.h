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

    FacialElasticEnergyEvaluation evaluate(
        const FacialElasticSystem &system,
        const std::vector<glm::dvec3> &positions) const;

    FacialElasticSolveResult solve(
        const FacialElasticSystem &system,
        const FacialElasticSolverSettings &settings = {},
        AdmissibilityCallback admissibility = {}) const;
};
