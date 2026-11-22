#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

// All geometry, forces and material coefficients use metres, newtons and pascals.
// A triangle's explicit material frame remains independent of the solve state.
struct FacialElasticTriangle
{
    std::array<std::size_t, 3> vertex_indices{};
    glm::dmat2 inverse_rest_matrix{1.0};
    double rest_area_m2 = 0.0;
    double young_modulus_pa = 0.0;
    double poisson_ratio = 0.0;
    double thickness_m = 0.0;
};

struct FacialElasticInsertionWeight
{
    std::size_t vertex_index = 0;
    double weight = 0.0;
};

struct FacialElasticMuscle
{
    std::string name;
    std::vector<FacialElasticInsertionWeight> insertions;
    glm::dvec3 origin{0.0};
    double rest_length_m = 0.0;
    double activation = 0.0;
    double maximum_shortening = 0.0;
    double stiffness_n_per_m = 0.0;
};

struct FacialElasticSystem
{
    std::vector<glm::dvec3> rest_positions;
    std::vector<FacialElasticTriangle> triangles;
    // Empty arrays disable the corresponding term. Otherwise they match the
    // number of positions. Fixed vertices remain exactly at rest_positions.
    std::vector<double> support_stiffness_n_per_m;
    std::vector<glm::dvec3> support_targets;
    std::vector<bool> fixed_vertices;
    std::vector<glm::dvec3> external_forces_n;
    std::vector<FacialElasticMuscle> muscles;
};

struct FacialElasticSolverSettings
{
    std::size_t max_iterations = 500;
    double relative_force_tolerance = 1.0e-4;
    std::size_t history_size = 8;
    std::size_t max_line_search_steps = 32;
};

struct FacialElasticMuscleState
{
    std::string name;
    glm::dvec3 insertion_position{0.0};
    double length_m = 0.0;
    double target_length_m = 0.0;
    double extension_m = 0.0;
    double tension_n = 0.0;
    double energy_j = 0.0;
};

struct FacialElasticEnergyEvaluation
{
    double total_energy_j = 0.0;
    double membrane_energy_j = 0.0;
    double support_energy_j = 0.0;
    double muscle_energy_j = 0.0;
    double external_potential_j = 0.0;
    // Maximum absolute eigenvalue of the 2x2 Green strain over all triangles.
    double maximum_green_strain = 0.0;
    // Derivative of total potential with respect to world-coordinate positions.
    // Entries at fixed vertices are retained so reactions can be inspected.
    std::vector<glm::dvec3> gradient_n;
    std::vector<glm::dvec3> support_forces_n;
    std::vector<double> muscle_lengths_m;
    std::vector<double> muscle_tensions_n;
    std::vector<FacialElasticMuscleState> muscle_states;
};

struct FacialElasticSolveResult
{
    std::vector<glm::dvec3> positions;
    bool converged = false;
    bool admissible = true;
    std::size_t iterations = 0;
    std::string termination_reason;
    double force_scale_n = 0.0;
    double free_force_l2_n = 0.0;
    double relative_free_force = 0.0;
    // Includes the initial energy followed only by accepted line-search states.
    std::vector<double> accepted_energies_j;
    FacialElasticEnergyEvaluation final_evaluation;
};
