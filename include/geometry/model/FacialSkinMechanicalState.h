#pragma once

#include "geometry/model/FacialElasticSystem.h"
#include "geometry/model/FacialBendingAssemblyDiagnostics.h"

#include <vector>

// The material system and its actual static solve travel with the generated
// mesh. Exporters can inspect the result without running a second simulation.
struct FacialSkinMechanicalState
{
	FacialElasticSystem system;
	FacialElasticSolveResult solve_result;
	FacialBendingAssemblyDiagnostics bending_assembly;
	std::vector<double> rest_vertex_areas_m2;
	std::vector<double> young_modulus_scales;
	std::vector<double> support_stiffness_scales;
};
