#pragma once

#include <cstddef>

// Evidence about the rest-coordinate reconstruction, separate from solver
// convergence and from any claim of anatomical or continuum calibration.
struct FacialBendingAssemblyDiagnostics
{
	std::size_t active_stencils = 0;
	std::size_t fixed_only_triangles = 0;
	// Subset of fixed_only_triangles: no valid reconstruction within the full
	// geometric search neighborhood, but every participating vertex is fixed.
	std::size_t fixed_only_unreconstructable_triangles = 0;
	std::size_t expanded_stencils = 0;
	std::size_t maximum_patch_vertices = 0;
	std::size_t maximum_neighborhood_rings = 0;
	double maximum_reconstruction_condition = 0.0;
	double maximum_polynomial_reproduction_error = 0.0;
	double maximum_rest_curvature_per_m = 0.0;
	double maximum_patch_height_ratio = 0.0;
};
