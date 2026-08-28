#pragma once

#include <cstddef>

class GeometryComplexityLimits
{
public:
	GeometryComplexityLimits(
		std::size_t maximum_profile_loops = 32,
		std::size_t maximum_points_per_profile = 1024,
		std::size_t maximum_loft_sections = 512,
		std::size_t maximum_path_points = 2048,
		std::size_t maximum_generated_vertices = 1000000,
		std::size_t maximum_generated_triangles = 2000000,
		std::size_t maximum_instance_array_count = 100000,
		std::size_t maximum_curve_control_points = 4096,
		std::size_t maximum_curve_samples = 16384,
		std::size_t maximum_surface_u_curves = 256,
		std::size_t maximum_surface_v_curves = 256,
		std::size_t maximum_surface_samples_per_axis = 2048)
		: maximum_profile_loops_(maximum_profile_loops),
		  maximum_points_per_profile_(maximum_points_per_profile),
		  maximum_loft_sections_(maximum_loft_sections),
		  maximum_path_points_(maximum_path_points),
		  maximum_generated_vertices_(maximum_generated_vertices),
		  maximum_generated_triangles_(maximum_generated_triangles),
		  maximum_instance_array_count_(maximum_instance_array_count),
		  maximum_curve_control_points_(maximum_curve_control_points),
		  maximum_curve_samples_(maximum_curve_samples),
		  maximum_surface_u_curves_(maximum_surface_u_curves),
		  maximum_surface_v_curves_(maximum_surface_v_curves),
		  maximum_surface_samples_per_axis_(maximum_surface_samples_per_axis)
	{
	}

	std::size_t maximumProfileLoops() const { return maximum_profile_loops_; }
	std::size_t maximumPointsPerProfile() const { return maximum_points_per_profile_; }
	std::size_t maximumLoftSections() const { return maximum_loft_sections_; }
	std::size_t maximumPathPoints() const { return maximum_path_points_; }
	std::size_t maximumGeneratedVertices() const { return maximum_generated_vertices_; }
	std::size_t maximumGeneratedTriangles() const { return maximum_generated_triangles_; }
	std::size_t maximumInstanceArrayCount() const { return maximum_instance_array_count_; }
	std::size_t maximumCurveControlPoints() const { return maximum_curve_control_points_; }
	std::size_t maximumCurveSamples() const { return maximum_curve_samples_; }
	std::size_t maximumSurfaceUCurves() const { return maximum_surface_u_curves_; }
	std::size_t maximumSurfaceVCurves() const { return maximum_surface_v_curves_; }
	std::size_t maximumSurfaceSamplesPerAxis() const
	{
		return maximum_surface_samples_per_axis_;
	}

private:
	std::size_t maximum_profile_loops_ = 32;
	std::size_t maximum_points_per_profile_ = 1024;
	std::size_t maximum_loft_sections_ = 512;
	std::size_t maximum_path_points_ = 2048;
	std::size_t maximum_generated_vertices_ = 1000000;
	std::size_t maximum_generated_triangles_ = 2000000;
	std::size_t maximum_instance_array_count_ = 100000;
	std::size_t maximum_curve_control_points_ = 4096;
	std::size_t maximum_curve_samples_ = 16384;
	std::size_t maximum_surface_u_curves_ = 256;
	std::size_t maximum_surface_v_curves_ = 256;
	std::size_t maximum_surface_samples_per_axis_ = 2048;
};
